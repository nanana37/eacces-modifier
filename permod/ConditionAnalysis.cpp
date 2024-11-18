//===-- ConditionAnalysis.cpp - Implement the ConditionAnalysis ------===//
//
// Analyze conditions that lead to an error
//

#include "llvm/IR/DebugInfoMetadata.h"

#include "Condition.hpp"
#include "ConditionAnalysis.hpp"
#include "ErrBBFinder.hpp"
#include "OriginFinder.hpp"
#include "debug.h"

using namespace llvm;
using namespace permod;

#define NONAME "UNNAMED CONDITION"

namespace ConditionAnalysis {

// ****************************************************************************
//                               Utility
// ****************************************************************************

/*
* Backtrace from load to store
* Get original variable (%0 is %flag)
* code example:
  store i32 %flag, ptr %flag.addr, align 4
  %0 = load i32, ptr %flag.addr, align 4
* TODO: may cause infinite loop
*/
Value *getOrigin(Value &V) {
  OriginFinder OF;
  Value *val = &V;

  for (int i = 0; i < MAX_TRACE_DEPTH; i++) {
    DEBUG_PRINT2("getOrigin: " << *val << "\n");
    if (!isa<Instruction>(val)) {
      return val;
    }
    Value *original = OF.visit(cast<Instruction>(val));
    if (!original) {
      return val;
    }
    val = original;
  }

  return val;
}

/*
 * Get struct name, if the V is a member of struct
 */
StringRef getStructName(Value &V) {
  if (!isa<GetElementPtrInst>(V))
    return "";
  Value *PtrOp = cast<GetElementPtrInst>(V).getPointerOperand();
  if (!PtrOp)
    return "";
  return getVarName(*PtrOp);
}

/*
 * Get variable name
 */
StringRef getVarName(Value &V) {
  StringRef name = getOrigin(V)->getName();
  if (name.empty())
    name = NONAME;
  if (name.endswith(".addr"))
    name = name.drop_back(5);

  DEBUG_PRINT2("name: " << name << "\n");
  return name;
}

/*
 * ****************************************************************************
 *                       Anlyzing Conditions
 * ****************************************************************************
 */

/*
 * Delete all conditions
 * Call this when you continue to next ErrBB
 */
void deleteAllCond(CondStack &Conds) {
  while (!Conds.empty()) {
    delete Conds.back();
    Conds.pop_back();
  }
}
bool isEmpty(CondStack &Conds) { return Conds.empty(); }

bool isBranchTrue(BranchInst &BrI, BasicBlock &DestBB) {
  if (BrI.getSuccessor(0) == &DestBB)
    return true;
  return false;
}

/* Get name & value of if condition
   - C
    if (flag & 2) {}     // name:of flag, val:val
   - IR
    %and = and i32 %flag, 2
    %tobool = icmp ne i32 %and, 0
    br i1 %tobool, label %if.then, label %if.end
 */
bool findIfCond_cmp(CondStack &Conds, BranchInst &BrI, CmpInst &CmpI,
                    BasicBlock &DestBB) {
  StringRef name;
  Value *val;
  CondType type;
  DEBUG_PRINT2("CMP: " << CmpI << "\n");

  Value *CmpOp = CmpI.getOperand(0);
  if (!CmpOp)
    return false;

  Value *CmpOp1 = CmpI.getOperand(1);
  if (!CmpOp1)
    return false;
  /* if cmp with null
   e.g. if (!inode):
      %tobool = icmp ne %inode, null
      br i1 %tobool, label %if.end, label %if.then
   */
  if (isa<ConstantPointerNull>(CmpOp1)) {
    name = getVarName(*CmpOp);
    val = CmpOp1;
    type = (isBranchTrue(BrI, DestBB) != CmpI.isFalseWhenEqual()) ? NLLTRU
                                                                  : NLLFLS;
    Conds.push_back(new Condition(name, val, type));
    return true;
  }

  // CmpOp: %and = and i32 %flag, 2
  if (auto *BinI = dyn_cast<BinaryOperator>(CmpOp)) {
    Value *flag;
    switch (BinI->getOpcode()) {
    case Instruction::BinaryOps::And:
      name = getVarName(*BinI);
      val = CmpOp1;
      flag = BinI->getOperand(1);
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? ANDTRU
                                                                    : ANDFLS;
      Conds.push_back(new Condition(name, val, type, flag));
      return true;
    default:
      DEBUG_PRINT("\n*************************************\n");
      DEBUG_PRINT("** Unexpected as BinI: " << *BinI << "\n");
      DEBUG_PRINT("Function: " << BrI.getFunction()->getName() << "\n");
      DEBUG_PRINT2("BB: " << *BrI.getParent() << "\n");
      DEBUG_PRINT("*************************************\n");
    }
  }

  // CmpOp: %call = call i32 @function()
  if (auto *CallI = dyn_cast<CallInst>(CmpOp)) {
    Function *Callee = CallI->getCalledFunction();
    if (!Callee)
      return false;

    name = getVarName(*Callee);
    val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), 0);

    // likely(a)/unlikely(a) macro become 'call llvm.expect(a, 1/0)'
    /* if (likely(dir_mode & 0002))
     *
if.end:                ; preds = %do.end
  %20 = load i16, ptr %dir_mode, align 2
  %conv34 = zext i16 %20 to i32
  %and35 = and i32 %conv34, 2
  %tobool36 = icmp ne i32 %and35, 0
  %lnot37 = xor i1 %tobool36, true
  %lnot39 = xor i1 %lnot37, true
  %lnot.ext40 = zext i1 %lnot39 to i32
  %conv41 = sext i32 %lnot.ext40 to i64
  %expval42 = call i64 @llvm.expect.i64(i64 %conv41, i64 1)
  %tobool43 = icmp ne i64 %expval42, 0
  br i1 %tobool43, label %if.then66, label %lor.lhs.false44
     */
    if (name.startswith("llvm.expect")) {
      DEBUG_PRINT2("llvm.expect\n");

      Value *arg1 = CallI->getArgOperand(1);
      if (isa<ConstantInt>(arg1)) {
        type = cast<ConstantInt>(arg1)->isZero() ? CALFLS : CALTRU;
      } else {
        DEBUG_PRINT("\n*************************************\n");
        DEBUG_PRINT("** Unexpected as arg1: " << *arg1 << "\n");
        DEBUG_PRINT("Function: " << BrI.getFunction()->getName() << "\n");
        DEBUG_PRINT2("BB: " << *BrI.getParent() << "\n");
        DEBUG_PRINT("*************************************\n");
        return false;
      }
      Conds.push_back(new Condition(name, cast<ConstantInt>(arg1), EXPECT));

      Value *arg0 = CallI->getArgOperand(0);
      arg0 = getOrigin(*arg0);

      if (isa<CmpInst>(arg0)) {
        // ex: if (likely(a > 0))
        findIfCond_cmp(Conds, BrI, cast<CmpInst>(*arg0), DestBB);
      } else if (isa<CallInst>(arg0)) {
        // ex: if (likely(func()))
        findIfCond_call(Conds, BrI, cast<CallInst>(*arg0), DestBB);
      } else {
        Conds.push_back(new Condition(getVarName(*arg0), arg1, type));
      }

      return true;
    }

    type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CALFLS
                                                                  : CALTRU;
    std::vector<ArgType> args;
    DEBUG_PRINT2("num of args: " << CallI->arg_size() << "\n");
    for (auto &arg : CallI->args()) {
      DEBUG_PRINT2("arg: " << *arg << "\n");
      StringRef name = getVarName(*arg);
      if (name.startswith(NONAME) && isa<ConstantInt>(arg)) {
        args.push_back(cast<ConstantInt>(arg));
      } else {
        args.push_back(getVarName(*arg));
      }
    }
    Conds.push_back(new Condition(name, val, type, args));
    return true;
  }

  // CmpOp: %1 = load i32, i32* %flag.addr, align 4
  // TODO: Try on some examples
  if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
    name = getVarName(*LoadI);
    val = CmpI.getOperand(1);

    type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CMPTRU
                                                                  : CMPFLS;
    Conds.push_back(new Condition(name, val, CmpI, isBranchTrue(BrI, DestBB)));

    return true;
  }
  DEBUG_PRINT("\n*************************************\n");
  DEBUG_PRINT("** Unexpected as CmpOp: " << *CmpOp << "\n");
  DEBUG_PRINT("Function: " << BrI.getFunction()->getName() << "\n");
  DEBUG_PRINT("BB: " << *BrI.getParent() << "\n");
  DEBUG_PRINT("*************************************\n");
  return false;
}

/* Get name & value of if condition
   - C
    if (!func()) {}   // name:of func, val:0
   - IR
    %call = call i32 @function()
    br i1 %call, label %if.then, label %if.end
 */
bool findIfCond_call(CondStack &Conds, BranchInst &BrI, CallInst &CallI,
                     BasicBlock &DestBB) {
  StringRef name;
  Value *val;
  CondType type;

  Function *Callee = CallI.getCalledFunction();
  if (!Callee)
    return false;

  name = getVarName(*Callee);
  val = ConstantInt::get(Type::getInt32Ty(CallI.getContext()), 0);
  type = isBranchTrue(BrI, DestBB) ? CALTRU : CALFLS;
  Conds.push_back(new Condition(name, val, type));
  return true;
}

/* Get name & value of if condition
   - And condition
    if (flag & 2) {}
   - Call condition
    if (!func()) {}   // name:of func, val:0
 */
bool findIfCond(CondStack &Conds, BranchInst &BrI, BasicBlock &DestBB) {

  // Branch sometimes has only one successor
  // e.g. br label %if.end
  if (!BrI.isConditional()) {
    DEBUG_PRINT2("Not a conditional branch\n");
    return true;
  }

  Value *IfCond = BrI.getCondition();
  if (!IfCond)
    return false;

  // And condition: if (flag & 2) {}
  if (isa<CmpInst>(IfCond)) {
    return findIfCond_cmp(Conds, BrI, cast<CmpInst>(*IfCond), DestBB);
  }
  // Call condition: if (!func()) {}
  if (isa<CallInst>(IfCond)) {
    return findIfCond_call(Conds, BrI, cast<CallInst>(*IfCond), DestBB);
  }
  DEBUG_PRINT("\n*************************************\n");
  DEBUG_PRINT("** Unexpected as IfCond: " << *IfCond << "\n");
  DEBUG_PRINT("Function: " << BrI.getFunction()->getName() << "\n");
  DEBUG_PRINT2("BB: " << *BrI.getParent() << "\n");
  DEBUG_PRINT("*************************************\n");
  return false;
}

/* Get name & value of switch condition
 * Returns: (StringRef, Value*)
   - switch (flag) {}     // name:of flag, val:of flag
 */
bool findSwCond(CondStack &Conds, SwitchInst &SwI) {
  StringRef name;
  Value *val;
  Value *SwCond = SwI.getCondition();

  // Get value
  val = SwCond;

  // Get name
  SwCond = getOrigin(*SwCond);
  if (!SwCond)
    return false;
  name = getVarName(*SwCond);

  Conds.push_back(new Condition(name, val, SWITCH));
  return true;
}

bool findConditions(CondStack &Conds, BasicBlock &CondBB, BasicBlock &DestBB) {
  DEBUG_PRINT2("\n** findConditions **\n");

  // Get Condition
  /*
   * if (flag & 2) {}     // name:of flag, val:val
   * switch (flag) {}     // name:of flag, val:of flag
   */
  if (auto *BrI = dyn_cast<BranchInst>(CondBB.getTerminator())) {
    DEBUG_PRINT2("Pred has BranchInst\n");
    return findIfCond(Conds, *BrI, DestBB);
  } else if (auto *SwI = dyn_cast<SwitchInst>(CondBB.getTerminator())) {
    DEBUG_PRINT2("Pred has SwitchInst\n");
    return findSwCond(Conds, *SwI);
  } else {
    DEBUG_PRINT2("* CondBB terminator is not a branch or switch\n");
    return false;
  }
}

// NOTE: need clang flag "-g"
void getDebugInfo(DebugInfo &DBinfo, Instruction &I, Function &F) {
  // DebugInfo: File name
  DBinfo.first = F.getParent()->getSourceFileName();

  // DebugInfo: Function name
  DBinfo.second = F.getName();

  // FIXME: show line number
  /*
  const DebugLoc &Loc = I.getDebugLoc();
  unsigned line = Loc->getLine();
  DEBUG_PRINT2("Got debug info: " << filename << ":" << line << "\n");
  Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
  */
}

void setRetCond(CondStack &Conds, BasicBlock &theBB) {
  Conds.push_back(new Condition("", NULL, _FUNC_));
}
} // namespace ConditionAnalysis
