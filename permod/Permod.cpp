#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"

#include "llvm/IR/InstVisitor.h"

#include <errno.h>
#include <llvm/IR/Instructions.h>
#include <unordered_set>

#define DEBUG
#ifdef DEBUG
#define MAX_TRACE_DEPTH 20
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
    errs() << x;                                                               \
  } while (0)
#else
#define MAX_TRACE_DEPTH 10
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif // DEBUG

/* #define TEST */
#ifdef TEST
#define LOGGER "printf"
#else
#define LOGGER "_printk"
#endif

using namespace llvm;

namespace {

#ifdef DEBUG
#define DEBUG_PRINT2(x)                                                        \
  do {                                                                         \
    if (!x)                                                                    \
      errs() << "(null)\n";                                                    \
    else if (isa<Function>(x))                                                 \
      errs() << "(Func)" << (x)->getName() << "\n";                            \
    else                                                                       \
      errs() << "(Val) " << *(x) << "\n";                                      \
  } while (0)
#else
#define DEBUG_PRINT2(x)                                                        \
  do {                                                                         \
  } while (0)
#endif // DEBUG

struct OriginFinder : public InstVisitor<OriginFinder, Value *> {
  Value *visitFunction(Function &F) { return nullptr; }
  // When visiting undefined by this visitor
  Value *visitInstruction(Instruction &I) { return nullptr; }

  Value *visitLoadInst(LoadInst &LI) { return LI.getPointerOperand(); }
  Value *visitStoreInst(StoreInst &SI) { return SI.getValueOperand(); }
  Value *visitZExtInst(ZExtInst &ZI) { return ZI.getOperand(0); }
  Value *visitSExtInst(SExtInst &SI) { return SI.getOperand(0); }
  // TODO: Is binary operator always and?
  Value *visitBinaryOperator(BinaryOperator &AI) { return AI.getOperand(0); }

  // NOTE: getCalledFunction() returns null for indirect call
  Value *visitCallInst(CallInst &CI) {
    if (CI.isIndirectCall()) {
      DEBUG_PRINT("It's IndirectCall\n");
      return CI.getCalledOperand();
    }
    /* if (CI.getCalledFunction()->getName().startswith("llvm")) { */
    /*   DEBUG_PRINT("It's LLVM intrinsic\n"); */
    /*   DEBUG_PRINT2(CI.getCalledOperand()); */
    /*   return CI.getCalledOperand(); */
    /* } */
    return CI.getCalledFunction();
  }

  // When facing %flag.addr, find below:
  // store %flag, ptr %flag.addr, align 4
  Value *visitAllocaInst(AllocaInst &AI) {
    if (AI.getName().endswith(".addr")) {
      for (User *U : AI.users()) {
        if (isa<StoreInst>(U)) {
          return U;
        }
      }
    }
    return nullptr;
  }
};

struct ConditionAnalysis {
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
  static Value *getOrigin(Value &V) {
    OriginFinder OF;
    Value *val = &V;

    for (int i = 0; i < MAX_TRACE_DEPTH; i++) {
      if (!isa<Instruction>(val)) {
        DEBUG_PRINT("** Not an instruction\n");
        return val;
      }
      Value *original = OF.visit(cast<Instruction>(val));
      if (!original) {
        DEBUG_PRINT("** No original\n");
        return val;
      }
      val = original;
    }

    DEBUG_PRINT("** Too deep for getOrigin\n");
    return val;
  }

  /*
   * Get struct name, if the V is a member of struct
   */
  static StringRef getStructName(Value &V) {
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
  static StringRef getVarName(Value &V) {
    StringRef Name = getOrigin(V)->getName();
    if (Name.empty())
      Name = "Unnamed Condition";
    DEBUG_PRINT("Name: " << Name << "\n");
    return Name;
  }

  /*
   * ****************************************************************************
   *                                Condition
   * ****************************************************************************
   */
  // Condition type: This is for type of log
  // NOTE: Used for array index!!
  enum CondType {
    CMPTRU = 0,
    CMPFLS,
    NLLTRU,
    NLLFLS,
    CALTRU,
    CALFLS,
    SWITCH,
    DBINFO,
    HELLOO,
    NUM_OF_CONDTYPE
  };

#ifdef DEBUG
  const char *condTypeStr[NUM_OF_CONDTYPE] = {"CMPTRU", "CMPFLS", "NLLTRU",
                                              "NLLFLS", "CALTRU", "CALFLS",
                                              "SWITCH", "DBINFO", "HELLOO"};
#endif // DEBUG

  class Condition {
  public:
    StringRef Name;
    Value *Val;
    CondType Type;

    Condition(StringRef name, Value *val, CondType type)
        : Name(name), Val(val), Type(type) {}
  };

  /*
   * Prepare format string
   */
  void prepareFormat(Value *format[], IRBuilder<> &builder, LLVMContext &Ctx) {
    StringRef formatStr[NUM_OF_CONDTYPE];
    formatStr[CMPTRU] = "[Permod] %s: %d\n";
    formatStr[CMPFLS] = "[Permod] %s: not %d\n";
    formatStr[NLLTRU] = "[Permod] %s: null\n";
    formatStr[NLLFLS] = "[Permod] %s: not null\n";
    formatStr[CALTRU] = "[Permod] %s(): not %d\n";
    formatStr[CALFLS] = "[Permod] %s(): %d\n";
    formatStr[SWITCH] = "[Permod] %s: %d (switch)\n";
    formatStr[DBINFO] = "[Permod] %s: %d\n";
    formatStr[HELLOO] = "--- Hello, I'm Permod ---\n";

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

  // Prepare Array of Condition
  std::vector<Condition *> conds;

  std::unordered_set<BasicBlock *> visitedBBs;

  // Check whether the DestBB is true or false successor
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
  bool findIfCond_cmp(BranchInst &BrI, CmpInst &CmpI, BasicBlock &DestBB) {
    StringRef name;
    Value *val;
    CondType type;

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
      val = CmpOp2;
      type = (isBranchTrue(BrI, DestBB) != CmpI.isFalseWhenEqual()) ? NLLTRU
                                                                    : NLLFLS;
      conds.push_back(new Condition(name, val, type));
      return true;
    }

    // CmpOp: %and = and i32 %flag, 2
    if (auto *AndI = dyn_cast<BinaryOperator>(CmpOp)) {
      CmpOp = AndI->getOperand(0);
      if (!CmpOp)
        return false;
      name = getVarName(*CmpOp);
      val = AndI->getOperand(1);
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CMPTRU
                                                                    : CMPFLS;
      conds.push_back(new Condition(name, val, type));
      return true;
    }

    // CmpOp: %call = call i32 @function()
    if (auto *CallI = dyn_cast<CallInst>(CmpOp)) {
      Function *Callee = CallI->getCalledFunction();
      if (!Callee)
        return false;

      name = getVarName(*Callee);
      val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), 0);
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CALTRU
                                                                    : CALFLS;
      conds.push_back(new Condition(name, val, type));
      return true;
    }

    // CmpOp: %1 = load i32, i32* %flag.addr, align 4
    // TODO: Try on some examples
    if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
      name = getVarName(*LoadI);
      val = CmpI.getOperand(1);

      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CMPTRU
                                                                    : CMPFLS;
      conds.push_back(new Condition(name, val, type));
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
  bool findIfCond_call(BranchInst &BrI, CallInst &CallI, BasicBlock &DestBB) {
    StringRef name;
    Value *val;
    CondType type;

    Function *Callee = CallI.getCalledFunction();
    if (!Callee)
      return false;

    name = getVarName(*Callee);
    val = ConstantInt::get(Type::getInt32Ty(CallI.getContext()), 0);
    type = isBranchTrue(BrI, DestBB) ? CALTRU : CALFLS;
    conds.push_back(new Condition(name, val, type));
    return true;
  }

  /* Get name & value of if condition
     - And condition
      if (flag & 2) {}
     - Call condition
      if (!func()) {}   // name:of func, val:0
   */
  bool findIfCond(BranchInst &BrI, BasicBlock &DestBB) {

    // Branch sometimes has only one successor
    // e.g. br label %if.end
    if (!BrI.isConditional()) {
      DEBUG_PRINT("Not a conditional branch\n");
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
  bool findSwCond(SwitchInst &SwI) {
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

    conds.push_back(new Condition(name, val, SWITCH));
    return true;
  }

  /*
   * Search predecessors for If/Switch Statement BB (CondBB)
   */
  void findPreds(BasicBlock &BB, std::vector<BasicBlock *> &preds) {
    DEBUG_PRINT("\n...Finding preds...\n");

    if (predecessors(&BB).empty())
      return;

    for (auto *PredBB : predecessors(&BB)) {
      DEBUG_PRINT("found a pred\n");
      Instruction *TI = PredBB->getTerminator();
      if (isa<BranchInst>(TI)) {
        preds.push_back(PredBB);
      } else if (isa<SwitchInst>(TI)) {
        /* caseBB may have multiple same preds
         * e.g.
            switch (x) {
            case A:
            case B:
              return -EACCES;
            }
        */
        if (std::find(preds.begin(), preds.end(), PredBB) == preds.end())
          preds.push_back(PredBB);
      }
    }
    DEBUG_PRINT("...End of Finding preds...\n");
  }

  bool findConditions(BasicBlock &CondBB, BasicBlock &DestBB) {
    DEBUG_PRINT("Trying to get conditions\n");

    // Get Condition
    /*
     * if (flag & 2) {}     // name:of flag, val:val
     * switch (flag) {}     // name:of flag, val:of flag
     */
    if (auto *BrI = dyn_cast<BranchInst>(CondBB.getTerminator())) {
      DEBUG_PRINT("Pred has BranchInst\n");
      return findIfCond(*BrI, DestBB);
    } else if (auto *SwI = dyn_cast<SwitchInst>(CondBB.getTerminator())) {
      DEBUG_PRINT("Pred has SwitchInst\n");
      return findSwCond(*SwI);
    } else {
      DEBUG_PRINT("* CondBB terminator is not a branch or switch\n");
      return false;
    }
  }

  // Search from bottom to top (entry block)
  void findAllConditions(BasicBlock &ErrBB, int depth = 0) {
    DEBUG_PRINT("\n*** findAllConditions ***\n");

    // Prevent infinite loop
    if (depth > MAX_TRACE_DEPTH) {
      DEBUG_PRINT("************** Too deep for findAllConditions\n");
      return;
    }

    // Record visited BBs to prevent infinite loop
    // Visited BB is the basic block whose preds are already checked
    // = All the conditions to the block are already found
    if (visitedBBs.find(&ErrBB) != visitedBBs.end()) {
      DEBUG_PRINT("************** Already visited\n");
      return;
    }
    visitedBBs.insert(&ErrBB);

    // Find CondBBs (conditional predecessors)
    std::vector<BasicBlock *> preds;
    findPreds(ErrBB, preds);
    if (preds.empty()) {
      DEBUG_PRINT("** No preds\n");
      return;
    }

    bool reachedEntry = true;
    for (auto *CondBB : preds) {
      if (!CondBB) {
        DEBUG_PRINT("*OMGOMGOMGOMG No CondBB in preds\n");
        continue;
      }

      DEBUG_PRINT("CondBB: " << CondBB->getName() << "\n");
      DEBUG_PRINT2(CondBB);

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
    }
  }

  /*
   * Delete all conditions
   * Call this when you continue to next ErrBB
   */
  void deleteAllCond() {
    while (!conds.empty()) {
      delete conds.back();
      conds.pop_back();
    }
  }

  // Debug info
  // NOTE: need clang flag "-g"
  void getDebugInfo(StoreInst &SI, Function &F) {
    LLVMContext &Ctx = SI.getContext();
    StringRef filename = F.getParent()->getSourceFileName();
    const DebugLoc &Loc = dyn_cast<Instruction>(&SI)->getDebugLoc();
    unsigned line = Loc->getLine();
    DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
    Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
    // The analyzing function
    conds.push_back(new Condition(
        F.getName(), dyn_cast<StoreInst>(&SI)->getValueOperand(), CALFLS));
    // File name and line number
    conds.push_back(new Condition(filename, lineVal, DBINFO));
    // Hello
    conds.push_back(new Condition("", NULL, HELLOO));
  }

  bool insertLoggers(BasicBlock *ErrBB, Function &F) {
    DEBUG_PRINT("\n...Inserting log...\n");
    bool modified = false;

    // Prepare builder
    IRBuilder<> builder(ErrBB);
    builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());
    LLVMContext &Ctx = ErrBB->getContext();

    // Prepare function
    std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
    Type *retType = Type::getVoidTy(Ctx);
    FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
    FunctionCallee logFunc =
        F.getParent()->getOrInsertFunction(LOGGER, funcType);

    // Prepare format
    Value *format[NUM_OF_CONDTYPE];
    prepareFormat(format, builder, Ctx);

    // Prepare arguments
    std::vector<Value *> args;

    while (!conds.empty()) {
      Condition *cond = conds.back();
      conds.pop_back();

      if (cond->Type == HELLOO) {
        args.push_back(format[cond->Type]);
        builder.CreateCall(logFunc, args);
        DEBUG_PRINT("Inserted log for HELLOO\n");
        args.clear();
        delete cond;
        continue;
      }

      // Insert log
      args.push_back(format[cond->Type]);
      args.push_back(builder.CreateGlobalStringPtr(cond->Name));
      args.push_back(cond->Val);
      builder.CreateCall(logFunc, args);

#ifdef DEBUG
      DEBUG_PRINT(condTypeStr[cond->Type] << " " << cond->Name << ": "
                                          << *cond->Val << "\n");
#endif // DEBUG

      args.clear();
      modified = true;
    }

    return modified;
  }

  bool isEmpty() { return conds.empty(); }
};

/*
 * Find 'return -EACCES'
    * The statement turns into:
    Error-thrower BB:
        store i32 -13，ptr %1, align 4
    Terminator BB:
        %2 = load i32, ptr %1, align 4
        ret i32 %2
 */

struct PermodPass : public PassInfoMixin<PermodPass> {
  /*
   * ****************************************************************************
   *                                Find 'return -EACCES'
   * ****************************************************************************
   */
  // NOTE: Expect as Function has only one ret inst
  Value *getReturnValue(Function &F) {
    for (auto &BB : F) {
      Value *RetVal = findReturnValue(BB);
      if (RetVal)
        return RetVal;
    }
    return nullptr;
  }

  // NOTE: maybe to find alloca is better?
  Value *findReturnValue(BasicBlock &BB) {
    Instruction *TI = BB.getTerminator();
    if (!TI)
      return nullptr;

    // Search for return inst
    ReturnInst *RI = dyn_cast<ReturnInst>(TI);
    if (!RI)
      return nullptr;

    // What is ret value?
    Value *RetVal = RI->getReturnValue();
    if (!RetVal)
      return nullptr;
    LoadInst *LI = dyn_cast<LoadInst>(RetVal);
    if (!LI) {
      return nullptr;
    }
    RetVal = LI->getPointerOperand();

    // RetVal should be Alloca or GEP
    /* DEBUG_PRINT("Type of RetVal is "); */
    /* if (isa<AllocaInst>(RetVal)) { */
    /*   DEBUG_PRINT("AllocaInst\n"); */
    /* } else { */
    /*   DEBUG_PRINT("not AllocaInst" << *RetVal << "\n"); */
    /* } */
    return RetVal;
  }

  bool isStoreErr(StoreInst &SI) {
    Value *ValOp = SI.getValueOperand();
    // NOTE: return -EACCES;
    if (auto *CI = dyn_cast<ConstantInt>(ValOp)) {
      if (CI->getSExtValue() == -EACCES)
        return true;
    }
    // NOTE: return (flag & 2) ? -EACCES : 0;
    // FIXME: Checking select inst will cause crash
    /* if (auto *SI = dyn_cast<SelectInst>(ValOp)) { */
    /*   if (auto *CI = dyn_cast<ConstantInt>(SI->getTrueValue())) { */
    /*     if (CI->getSExtValue() == -EACCES) */
    /*       return true; */
    /*   } */
    /*   if (auto *CI = dyn_cast<ConstantInt>(SI->getFalseValue())) { */
    /*     if (CI->getSExtValue() == -EACCES) */
    /*       return true; */
    /*   } */
    /* } */
    return false;
  }

  /* Get error-thrower BB "ErrBB"
     store i32 -13，ptr %1, align 4
   * Returns: BasicBlock*
   */
  BasicBlock *getErrBB(StoreInst &SI) {
    if (!isStoreErr(SI))
      return nullptr;

    return SI.getParent();
  }

  bool isStorePtr(StoreInst &SI) {
    return SI.getValueOperand()->getType()->isPointerTy();
  }

  // Get error value (for ERR_PTR)
  // e.g.,
  // %error = alloca i32, align 4, !DIAssignID !8288
  // %103 = load i32, ptr %error, align 4, !dbg !8574
  // %conv156 = sext i32 %103 to i64, !dbg !8574
  // %call157 = call ptr @ERR_PTR(i64 noundef %conv156) #22,
  // store ptr %call157, ptr %retval, align 8, !dbg !8576
  Value *getErrValue(StoreInst &SI) {
    DEBUG_PRINT("\ngetErrValue of " << SI << "\n");

    CallInst *CI = dyn_cast<CallInst>(SI.getValueOperand());
    if (!CI)
      return nullptr;

    // NOTE: this function only checks ERR_PTR(x)
    if (CI->arg_size() != 1)
      return nullptr;

    Value *ErrVal = CI->getArgOperand(0);
    if (!ErrVal)
      return nullptr;

    OriginFinder OF;
    for (int i = 0; i < MAX_TRACE_DEPTH; i++) {

      if (!isa<Instruction>(ErrVal))
        break;
      if (isa<AllocaInst>(ErrVal))
        break;
      if (isa<SelectInst>(ErrVal))
        break;
      Value *newVal = OF.visit(*dyn_cast<Instruction>(ErrVal));
      if (!newVal)
        break;
      ErrVal = newVal;
    }

    DEBUG_PRINT("getErrValue ends: " << *ErrVal << "\n");

    return ErrVal;
  }

  bool analysisForPtr(StoreInst &SI, Function &F) {
    DEBUG_PRINT("\n==AnalysisForPtr==\n");
    bool modified = false;

    Value *ErrVal = getErrValue(SI);
    if (!ErrVal)
      return false;

    DEBUG_PRINT2(ErrVal);

    for (User *U : ErrVal->users()) {
      StoreInst *SI = dyn_cast<StoreInst>(U);
      if (!SI)
        continue;
      BasicBlock *ErrBB = getErrBB(*SI);
      if (!ErrBB)
        continue;
      DEBUG_PRINT("\n///////////////////////////////////////\n");
      DEBUG_PRINT(F.getName() << " has 'return -EACCES'\n");
      DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");

      struct ConditionAnalysis ConditionAnalysis {};

      // Backtrace to find If/Switch Statement BB
      ConditionAnalysis.findAllConditions(*ErrBB);
      if (ConditionAnalysis.isEmpty()) {
        DEBUG_PRINT("** conds is empty\n");
        continue;
      }

      ConditionAnalysis.getDebugInfo(*SI, F);

      // Insert loggers
      modified |= ConditionAnalysis.insertLoggers(ErrBB, F);
      if (ConditionAnalysis.isEmpty()) {
        DEBUG_PRINT("~~~ Inserted all logs ~~~\n\n");
      } else {
        DEBUG_PRINT("** Failed to insert all logs\n");
        ConditionAnalysis.deleteAllCond();
      }
    }
    DEBUG_PRINT("modified: " << modified << "\n");
    return modified;
  }

  bool analysisForInt32(StoreInst &SI, Function &F) {
    DEBUG_PRINT("\n==AnalysisForInt32==\n");
    bool modified = false;

    // Get Error-thrower BB "ErrBB"
    /* store -13, ptr %1, align 4 */
    BasicBlock *ErrBB = getErrBB(SI);
    if (!ErrBB)
      return false;

    DEBUG_PRINT("\n///////////////////////////////////////\n");
    DEBUG_PRINT(F.getName() << " has 'return -EACCES'\n");
    DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");

    struct ConditionAnalysis ConditionAnalysis {};

    // Backtrace to find If/Switch Statement BB
    ConditionAnalysis.findAllConditions(*ErrBB);
    if (ConditionAnalysis.isEmpty()) {
      DEBUG_PRINT("** conds is empty\n");
      return false;
    }

    ConditionAnalysis.getDebugInfo(SI, F);

    // Insert loggers
    modified |= ConditionAnalysis.insertLoggers(ErrBB, F);
    if (ConditionAnalysis.isEmpty()) {
      DEBUG_PRINT("~~~ Inserted all logs ~~~\n\n");
    } else {
      DEBUG_PRINT("** Failed to insert all logs\n");
      ConditionAnalysis.deleteAllCond();
    }

    DEBUG_PRINT("modified: " << modified << "\n");
    return modified;
  }

  /*
   *****************
   * main function *
   *****************
   */
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    DEBUG_PRINT("\n@@@ PermodPass @@@\n");
    DEBUG_PRINT("Module: " << M.getName() << "\n");
    bool modified = false;
    for (auto &F : M.functions()) {
      DEBUG_PRINT("\n-FUNCTION: " << F.getName() << "\n");
      /* if (F.getName() == "lookup_open") */
      /*   DEBUG_PRINT(F); */

      // Skip
      if (F.isDeclaration()) {
        DEBUG_PRINT("--- Skip Declaration\n");
        continue;
      }
      if (F.getName().startswith("llvm")) {
        DEBUG_PRINT("--- Skip llvm\n");
        continue;
      }
      if (F.getName() == LOGGER) {
        DEBUG_PRINT("--- Skip Logger\n");
        continue;
      }

      // FIXME: this function cause crash
      if (F.getName() == "profile_transition") {
        DEBUG_PRINT("--- Skip profile_transition\n");
        continue;
      }

      Value *RetVal = getReturnValue(F);
      if (!RetVal)
        continue;
      DEBUG_PRINT("Return value: " << *RetVal << "\n\n");

      // Analysis for ERR_PTR
      // Search again, if RetVal is alloca ptr
      if (auto *AllocaI = dyn_cast<AllocaInst>(RetVal)) {
        DEBUG_PRINT("RetVal is AllocaInst\n");
        // alloca i32, align 4
        if (AllocaI->getAllocatedType() == Type::getInt32Ty(F.getContext())) {
          DEBUG_PRINT("RetVal is " << *AllocaI->getAllocatedType() << "\n");
        }
        // alloca ptr, align 8
        if (AllocaI->getAllocatedType() == Type::getInt8PtrTy(F.getContext())) {
          DEBUG_PRINT("RetVal is " << *AllocaI->getAllocatedType() << "\n");
          /* RetVal = findReturnValue(*AllocaI->getParent()); */
        }
      }

      for (User *U : RetVal->users()) {
        StoreInst *SI = dyn_cast<StoreInst>(U);
        if (!SI)
          continue;
        DEBUG_PRINT("StoreInst: " << *SI << "\n");

        if (isStorePtr(*SI)) {
          modified |= analysisForPtr(*SI, F);
        } else {
          modified |= analysisForInt32(*SI, F);
        }
      }
    }
    if (modified) {
      DEBUG_PRINT("Modified\n");
      return PreservedAnalyses::none();
    } else {
      DEBUG_PRINT("Not Modified\n");
      return PreservedAnalyses::all();
    }
  };
}; // namespace

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Permod pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(PermodPass());
                });
          }};
}
