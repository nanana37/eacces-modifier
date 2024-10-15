//===-- ConditionAnalysis.cpp - Implement the ConditionAnalysis ------===//
//
// Analyze conditions that lead to an error
//

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"

#include "Condition.h"
#include "ConditionAnalysis.h"
#include "ErrBBFinder.h"
#include "OriginFinder.h"
#include "debug.h"

using namespace llvm;
using namespace permod;

#define NONAME "UNNAMED CONDITION"

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
Value *ConditionAnalysis::getOrigin(Value &V) {
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
StringRef ConditionAnalysis::getStructName(Value &V) {
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
StringRef ConditionAnalysis::getVarName(Value &V) {
  StringRef name = getOrigin(V)->getName();
  if (name.empty())
    name = NONAME;
  if (name.endswith(".addr"))
    name = name.drop_back(5);

  DEBUG_PRINT2("name: " << name << "\n");
  return name;
}

/*
 * Prepare format string
 */
void ConditionAnalysis::prepFormat() {
  StringRef formatStr[NUM_OF_CONDTYPE];
  formatStr[CMPTRU] = "[Permod] %s == %d is %s\n";
  formatStr[CMPFLS] = "[Permod] %s != %d is %s\n";
  formatStr[CMP_GT] = "[Permod] %s > %d is %s\n";
  formatStr[CMP_GE] = "[Permod] %s >= %d is %s\n";
  formatStr[CMP_LT] = "[Permod] %s < %d is %s\n";
  formatStr[CMP_LE] = "[Permod] %s <= %d is %s\n";
  formatStr[NLLTRU] = "[Permod] %s == null is %s\n";
  formatStr[NLLFLS] = "[Permod] %s != null is %s\n";
  formatStr[CALTRU] = "[Permod] %s() != %d is %s\n";
  formatStr[CALFLS] = "[Permod] %s() == %d is %s\n";
  formatStr[ANDTRU] = "[Permod] %s &== %d is %s\n";
  formatStr[ANDFLS] = "[Permod] %s &!= %d is %s\n";
  formatStr[SWITCH] = "[Permod] %s == %d (switch)\n";
  formatStr[EXPECT] = "[Permod] %s expect %d\n";
  formatStr[DBINFO] = "[Permod] %s: %d\n";
  formatStr[HELLOO] = "--- Hello, I'm Permod ---\n";
  formatStr[_OPEN_] = "[Permod] {\n";
  formatStr[_CLSE_] = "[Permod] }\n";
  formatStr[_TRUE_] = "true";
  formatStr[_FLSE_] = "false";
  formatStr[RETURN] = "[Permod] %d is returned\n";
  formatStr[_VARS_] = "[Permod] %s\n";
  formatStr[_VARC_] = "[Permod] %d\n";

  for (int i = 0; i < NUM_OF_CONDTYPE; i++) {
    Value *formatVal = Builder.CreateGlobalStringPtr(formatStr[i]);
    Format[i] = Builder.CreatePointerCast(formatVal, Type::getInt8PtrTy(Ctx));
  }
}

void ConditionAnalysis::prepLogger() {
  // Prepare function
  std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  LogFunc = TargetFunc->getParent()->getOrInsertFunction(LOGGER, funcType);
}

/*
 * ****************************************************************************
 *                       Anlyzing Conditions
 * ****************************************************************************
 */

/* Get name & value of if condition
   - C
    if (flag & 2) {}     // name:of flag, val:val
   - IR
    %and = and i32 %flag, 2
    %tobool = icmp ne i32 %and, 0
    br i1 %tobool, label %if.then, label %if.end
 */
bool ConditionAnalysis::findIfCond_cmp(BranchInst &BrI, CmpInst &CmpI,
                                       BasicBlock &DestBB) {
  StringRef name;
  Value *val;
  CondType type;
  DEBUG_PRINT2("CMP: " << *CmpI.getParent() << "\n");

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
    switch (BinI->getOpcode()) {
    case Instruction::BinaryOps::And:
      name = getVarName(*BinI);
      val = BinI->getOperand(1);
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? ANDTRU
                                                                    : ANDFLS;
      Conds.push_back(new Condition(name, val, type));
      return true;
    default:
      DEBUG_PRINT("** Unexpected as BinI: " << *BinI << "\n");
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
        DEBUG_PRINT("** Unexpected as arg1: " << *arg1 << "\n");
        return false;
      }
      Conds.push_back(new Condition(name, cast<ConstantInt>(arg1), EXPECT));

      Value *arg0 = CallI->getArgOperand(0);
      arg0 = getOrigin(*arg0);

      if (isa<CmpInst>(arg0)) {
        // ex: if (likely(a > 0))
        findIfCond_cmp(BrI, cast<CmpInst>(*arg0), DestBB);
      } else if (isa<CallInst>(arg0)) {
        // ex: if (likely(func()))
        findIfCond_call(BrI, cast<CallInst>(*arg0), DestBB);
      } else {
        Conds.push_back(new Condition(getVarName(*arg0), arg1, type));
      }

      return true;
    }

    type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CALTRU
                                                                  : CALFLS;
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
    // Conds.push_back(new Condition(name, val, type));
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

  DEBUG_PRINT("** Unexpected as CmpOp: " << *CmpOp << "\n");
  return false;
}

/* Get name & value of if condition
   - C
    if (!func()) {}   // name:of func, val:0
   - IR
    %call = call i32 @function()
    br i1 %call, label %if.then, label %if.end
 */
bool ConditionAnalysis::findIfCond_call(BranchInst &BrI, CallInst &CallI,
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
bool ConditionAnalysis::findIfCond(BranchInst &BrI, BasicBlock &DestBB) {

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
    return findIfCond_cmp(BrI, cast<CmpInst>(*IfCond), DestBB);
  }
  // Call condition: if (!func()) {}
  if (isa<CallInst>(IfCond)) {
    return findIfCond_call(BrI, cast<CallInst>(*IfCond), DestBB);
  }
  DEBUG_PRINT("** Unexpected as IfCond: " << *IfCond << "\n");
  return false;
}

/* Get name & value of switch condition
 * Returns: (StringRef, Value*)
   - switch (flag) {}     // name:of flag, val:of flag
 */
bool ConditionAnalysis::findSwCond(SwitchInst &SwI) {
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

bool ConditionAnalysis::findConditions(BasicBlock &CondBB, BasicBlock &DestBB) {
  DEBUG_PRINT2("\n** findConditions **\n");

  // Get Condition
  /*
   * if (flag & 2) {}     // name:of flag, val:val
   * switch (flag) {}     // name:of flag, val:of flag
   */
  if (auto *BrI = dyn_cast<BranchInst>(CondBB.getTerminator())) {
    DEBUG_PRINT2("Pred has BranchInst\n");
    return findIfCond(*BrI, DestBB);
  } else if (auto *SwI = dyn_cast<SwitchInst>(CondBB.getTerminator())) {
    DEBUG_PRINT2("Pred has SwitchInst\n");
    return findSwCond(*SwI);
  } else {
    DEBUG_PRINT2("* CondBB terminator is not a branch or switch\n");
    return false;
  }
}

// Debug info
// NOTE: need clang flag "-g"
void ConditionAnalysis::getDebugInfo(Instruction &I, Function &F) {
  // The analyzing function
  ErrBBFinder EBF;
  if (auto val = EBF.getErrno(I)) {
    Conds.push_back(new Condition(F.getName(), val, CALFLS));
    DEBUG_PRINT("ERRNO: " << F.getName() << " " << *val << "\n");
  }

  LLVMContext &Ctx = I.getContext();
  StringRef filename = F.getParent()->getSourceFileName();
  const DebugLoc &Loc = I.getDebugLoc();
  unsigned line = Loc->getLine();
  DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
  Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
  // File name and line number
  Conds.push_back(new Condition(filename, lineVal, DBINFO));
  // Hello
  // Conds.push_back(new Condition("", NULL, HELLOO));
}

void ConditionAnalysis::setRetCond(BasicBlock &theBB) {
  Conds.push_back(new Condition("", NULL, RETURN));
}

bool ConditionAnalysis::insertLoggers(BasicBlock &theBB) {
  DEBUG_PRINT("\n...Inserting log...\n");
  bool modified = false;

  // Insert just before the terminator
  Builder.SetInsertPoint(theBB.getTerminator());
  Value *termC = theBB.getTerminator()->getOperand(0);
  if (!termC) {
    DEBUG_PRINT("** Condition of terminator is NULL\n");
    return false;
  }

  // TODO: This must be useful, but logs become long.
  // getDebugInfo(*ErrBB.getTerminator(), theBB.getParent());

  // Prepare arguments
  std::vector<Value *> args;

  while (!Conds.empty()) {
    Condition *cond = Conds.back();
    Conds.pop_back();

    args.push_back(Format[cond->getType()]);
    DEBUG_PRINT(condTypeStr[cond->getType()]);

    switch (cond->getType()) {
    case RETURN:
      args.push_back(termC);
      break;
    case HELLOO:
    case _OPEN_:
    case _CLSE_:
      DEBUG_PRINT("\n");
      break;
    case SWITCH:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      // args.push_back(cond->getConst());
      args.push_back(termC);
      break;
    default:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(cond->getConst());
      Value *newSel =
          Builder.CreateSelect(termC, Format[_TRUE_], Format[_FLSE_]);
      args.push_back(newSel);
      break;
    }

    Builder.CreateCall(LogFunc, args);
    args.clear();

    if (cond->getType() == CALTRU || cond->getType() == CALFLS) {
      args.push_back(Format[_OPEN_]);
      Builder.CreateCall(LogFunc, args);
      args.clear();
      for (auto arg : cond->getArgs()) {
        if (auto s = std::get_if<StringRef>(&arg)) {
          args.push_back(Format[_VARS_]);
          args.push_back(Builder.CreateGlobalStringPtr(*s));
        } else {
          args.push_back(Format[_VARC_]);
          args.push_back(std::get<ConstantInt *>(arg));
        }
        Builder.CreateCall(LogFunc, args);
        args.clear();
      }
      args.push_back(Format[_CLSE_]);
      Builder.CreateCall(LogFunc, args);
      args.clear();
    }

    delete cond;
    modified = true;
  }

  Builder.ClearInsertionPoint();

  return modified;
}
