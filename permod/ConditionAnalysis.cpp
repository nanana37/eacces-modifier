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
  StringRef Name = getOrigin(V)->getName();
  if (Name.empty())
    Name = "Unnamed Condition";
  DEBUG_PRINT2("Name: " << Name << "\n");
  return Name;
}

/*
 * Prepare format string
 */
void ConditionAnalysis::prepareFormat(Value *format[], IRBuilder<> &builder,
                                      LLVMContext &Ctx) {
  StringRef formatStr[NUM_OF_CONDTYPE];
  formatStr[CMPTRU] = "[Permod] %s:%d == %d\n";
  formatStr[CMPFLS] = "[Permod] %s:%d != %d\n";
  formatStr[CMP_GT] = "[Permod] %s:%d > %d\n";
  formatStr[CMP_GE] = "[Permod] %s:%d >= %d\n";
  formatStr[CMP_LT] = "[Permod] %s:%d < %d\n";
  formatStr[CMP_LE] = "[Permod] %s:%d <= %d\n";
  formatStr[CMP_XX] = "[Permod] %s:%d vs %d\n";
  formatStr[NLLTRU] = "[Permod] %s:%d == null\n";
  formatStr[NLLFLS] = "[Permod] %s:%d != null\n";
  formatStr[CALTRU] = "[Permod] %s():%d != %d\n";
  formatStr[CALFLS] = "[Permod] %s():%d == %d\n";
  formatStr[ANDTRU] = "[Permod] %s:%d & %d > 0\n";
  formatStr[ANDFLS] = "[Permod] %s:%d & %d == 0\n";
  formatStr[SWITCH] = "[Permod] switch (%s:%d)\n";
  formatStr[DBINFO] = "[Permod] %s: %d\n";
  formatStr[ERRNOM] = "[Permod] %s() returned %d (errno)\n";
  formatStr[HELLOO] = "--- Hello, I'm Permod ---\n";
  formatStr[_OPEN_] = "[Permod] {\n";
  formatStr[_CLSE_] = "[Permod] }\n";

  for (int i = 0; i < NUM_OF_CONDTYPE; i++) {
    Value *formatVal = builder.CreateGlobalStringPtr(formatStr[i]);
    format[i] = builder.CreatePointerCast(formatVal, Type::getInt8PtrTy(Ctx));
  }
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
  Value *var;
  Value *con;
  CondType type;
  DEBUG_PRINT2("CMP: " << *CmpI.getParent() << "\n");

  Value *CmpOp = CmpI.getOperand(0);
  if (!CmpOp)
    return false;

  Value *CmpOp2 = CmpI.getOperand(1);
  if (!CmpOp2)
    return false;
  /* if cmp with null
   e.g. if (!inode):
      %tobool = icmp ne %inode, null
      br i1 %tobool, label %if.end, label %if.then
   */
  if (isa<ConstantPointerNull>(CmpOp2)) {
    name = getVarName(*CmpOp);
    var = CmpOp2;
    type = (isBranchTrue(BrI, DestBB) != CmpI.isFalseWhenEqual()) ? NLLTRU
                                                                  : NLLFLS;
    conds.push_back(new Condition(name, var, type));
    return true;
  }

  // CmpOp: %and = and i32 %flag, 2
  if (auto *BinI = dyn_cast<BinaryOperator>(CmpOp)) {
    switch (BinI->getOpcode()) {
    case Instruction::BinaryOps::And:
      name = getVarName(*BinI);
      var = CmpOp;
      con = CmpOp2;
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? ANDTRU
                                                                    : ANDFLS;
      conds.push_back(new Condition(name, var, con, type));
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
    var = CmpOp;
    con = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), 0);
    type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CALTRU
                                                                  : CALFLS;
    conds.push_back(
        new Condition(name, var, con, CmpI, isBranchTrue(BrI, DestBB)));
    return true;
  }

  // CmpOp: %1 = load i32, i32* %flag.addr, align 4
  // TODO: Try on some examples
  if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
    name = getVarName(*LoadI);
    var = CmpOp;
    con = CmpOp2;

    type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CMPTRU
                                                                  : CMPFLS;
    conds.push_back(
        new Condition(name, var, con, CmpI, isBranchTrue(BrI, DestBB)));

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
  Value *var;
  Value *con;
  CondType type;

  Function *Callee = CallI.getCalledFunction();
  if (!Callee)
    return false;

  name = getVarName(*Callee);
  var = &CallI;
  con = ConstantInt::get(Type::getInt32Ty(CallI.getContext()), 0);
  type = isBranchTrue(BrI, DestBB) ? CALTRU : CALFLS;
  conds.push_back(new Condition(name, var, con, type));
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
  Value *con;
  Value *SwCond = SwI.getCondition();

  // Get value
  con = SwCond;

  // Get name
  SwCond = getOrigin(*SwCond);
  if (!SwCond)
    return false;
  name = getVarName(*SwCond);

  conds.push_back(new Condition(name, con, SWITCH));
  return true;
}

/*
 * Search predecessors for If/Switch Statement BB (CondBB)
 */
void ConditionAnalysis::findPreds(BasicBlock &BB,
                                  std::vector<BasicBlock *> &preds) {
  DEBUG_PRINT2("\n...Finding preds...\n");

  if (predecessors(&BB).empty())
    return;

  for (auto *PredBB : predecessors(&BB)) {
    DEBUG_PRINT2("found a pred\n");
    Instruction *TI = PredBB->getTerminator();
    if (isa<BranchInst>(TI)) {
      preds.push_back(PredBB);
    } else if (isa<SwitchInst>(TI)) {
      /* caseBB may have multiple same preds
       * e.g.
          switch (x) {
          case A:
          case B:
            return -ERRNO;
          }
      */
      if (std::find(preds.begin(), preds.end(), PredBB) == preds.end())
        preds.push_back(PredBB);
    }
  }
  DEBUG_PRINT2("...End of Finding preds...\n");
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

// Search from bottom to top (entry block)
void ConditionAnalysis::findAllConditions(BasicBlock &ErrBB, int depth) {
  DEBUG_PRINT2("\n*** findAllConditions ***\n");

  // Prevent infinite loop
  if (depth > MAX_TRACE_DEPTH) {
    DEBUG_PRINT("************** Too deep for findAllConditions\n");
    return;
  }

  // Record visited BBs to prevent infinite loop
  // Visited BB is the basic block whose preds are already checked
  // = All the conditions to the block are already found
  if (visitedBBs.find(&ErrBB) != visitedBBs.end()) {
    DEBUG_PRINT2("************** Already visited\n");
    return;
  }
  visitedBBs.insert(&ErrBB);

  // Find CondBBs (conditional predecessors)
  std::vector<BasicBlock *> preds;
  findPreds(ErrBB, preds);
  if (preds.empty()) {
    return;
  }

  bool reachedEntry = true;
  for (auto *CondBB : preds) {
    conds.push_back(new Condition("", NULL, _CLSE_));
    if (!CondBB) {
      DEBUG_PRINT("*OMGOMGOMGOMG No CondBB in preds\n");
      continue;
    }

    DEBUG_PRINT2("CondBB: " << CondBB->getName() << "\n");
    DEBUG_VALUE(CondBB);

    // Get Condition
    // TODO: Should this return bool?
    if (!findConditions(*CondBB, ErrBB)) {
      DEBUG_PRINT("*** findCond has failed.\n");
    }

    // NOTE: RECURSION
    findAllConditions(*CondBB, depth++);

    if (CondBB == &CondBB->getParent()->getEntryBlock()) {
      DEBUG_PRINT("* Reached to the entry\n");
      reachedEntry &= true;
    } else {
      reachedEntry &= false;
    }
    conds.push_back(new Condition("", NULL, _OPEN_));
  }
}

// Debug info
// NOTE: need clang flag "-g"
void ConditionAnalysis::getDebugInfo(Instruction &I, Function &F) {
  // The analyzing function
  ErrBBFinder EBF;
  if (auto val = EBF.getErrno(I)) {
    conds.push_back(new Condition(F.getName(), val, ERRNOM));
    DEBUG_PRINT("ERRNO: " << F.getName() << " " << *val << "\n");
  }

  LLVMContext &Ctx = I.getContext();
  StringRef filename = F.getParent()->getSourceFileName();
  const DebugLoc &Loc = I.getDebugLoc();
  unsigned line = Loc->getLine();
  DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
  Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
  // File name and line number
  conds.push_back(new Condition(filename, lineVal, DBINFO));
  // Hello
  conds.push_back(new Condition("", NULL, HELLOO));
}

bool ConditionAnalysis::insertLoggers(BasicBlock &ErrBB, Function &F) {
  DEBUG_PRINT("\n...Inserting log...\n");
  bool modified = false;

  // Prepare builder
  IRBuilder<> builder(&ErrBB);
  builder.SetInsertPoint(&ErrBB, ErrBB.getFirstInsertionPt());
  LLVMContext &Ctx = ErrBB.getContext();

  // Prepare function
  std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  FunctionCallee logFunc = F.getParent()->getOrInsertFunction(LOGGER, funcType);

  // Prepare format
  Value *format[NUM_OF_CONDTYPE];
  prepareFormat(format, builder, Ctx);

  // Prepare arguments
  std::vector<Value *> args;

  while (!conds.empty()) {
    Condition *cond = conds.back();
    conds.pop_back();

    args.push_back(format[cond->getType()]);
    DEBUG_PRINT(condTypeStr[cond->getType()]);

    switch (cond->getType()) {
    case CMPTRU:
    case CMPFLS:
    case CMP_GT:
    case CMP_GE:
    case CMP_LT:
    case CMP_LE:
    case CMP_XX:
    case CALTRU:
    case CALFLS:
    case ANDTRU:
    case ANDFLS:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getVar()
                      << *cond->getConst() << "\n");
      args.push_back(builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(cond->getVar());
      args.push_back(cond->getConst());
      break;
    case NLLTRU:
    case NLLFLS:
    case SWITCH:
    case DBINFO:
    case ERRNOM:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getVar() << "\n");
      args.push_back(builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(cond->getVar());
      break;
    case HELLOO:
    case _OPEN_:
    case _CLSE_:
      DEBUG_PRINT("\n");
      break;
    default:
      DEBUG_PRINT("what is this?\n");
    }

    builder.CreateCall(logFunc, args);
    args.clear();
    delete cond;
    modified = true;
  }

  return modified;
}

bool ConditionAnalysis::main(BasicBlock &ErrBB, Function &F, Instruction &I) {
  bool modified = false;

  // Backtrace to find If/Switch Statement BB
  findAllConditions(ErrBB);
  if (isEmpty()) {
    DEBUG_PRINT("** conds is empty\n");
    return false;
  }

  getDebugInfo(I, F);

  // Insert loggers
  modified = insertLoggers(ErrBB, F);
  if (isEmpty()) {
    DEBUG_PRINT("~~~ Inserted all logs ~~~\n\n");
  } else {
    DEBUG_PRINT("** Failed to insert all logs\n");
    deleteAllCond();
  }

  return modified;
}
