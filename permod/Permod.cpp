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

#define DEBUG
#ifdef DEBUG
#define MAX_TRACE_DEPTH 100
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

// TODO
#ifdef DEBUG
#define PRINT_VALUE(s, x)                                                      \
  do {                                                                         \
    errs() << s << " ";                                                        \
    if (!x)                                                                    \
      errs() << "null\n";                                                      \
    else if (isa<Function>(x))                                                 \
      errs() << "Function: " << x.getName() << "\n";                           \
    else                                                                       \
      errs() << "Value: " << x << "\n";                                        \
  } while (0)
#else
#define PRINT_VALUE(s, x)                                                      \
  do {                                                                         \
  } while (0)
#endif // DEBUG

struct OriginFinder : public InstVisitor<OriginFinder, Value *> {
  Value *visitFunction(Function &F) {
    DEBUG_PRINT("Visiting Function: " << F.getName() << "\n");
    return nullptr;
  }
  // When visiting undefined by this visitor
  Value *visitInstruction(Instruction &I) {
    DEBUG_PRINT("Visiting Instruction: " << I << "\n");
    return nullptr;
  }
  Value *visitLoadInst(LoadInst &LI) {
    DEBUG_PRINT("Visiting Load\n");
    return LI.getPointerOperand();
  }
  // NOTE: getCalledFunction() returns null for indirect call
  Value *visitCallInst(CallInst &CI) {
    DEBUG_PRINT("Visiting Call\n");
    if (CI.isIndirectCall()) {
      DEBUG_PRINT("It's IndirectCall\n");
      return CI.getCalledOperand();
    }
    return CI.getCalledFunction();
  }
  Value *visitStoreInst(StoreInst &SI) {
    DEBUG_PRINT("Visiting Store\n");
    return SI.getValueOperand();
  }
  Value *visitZExtInst(ZExtInst &ZI) {
    DEBUG_PRINT("Visiting ZExt\n");
    return ZI.getOperand(0);
  }
  Value *visitSExtInst(SExtInst &SI) {
    DEBUG_PRINT("Visiting SExt\n");
    return SI.getOperand(0);
  }
  // TODO: Is binary operator always and?
  Value *visitBinaryOperator(BinaryOperator &AI) {
    DEBUG_PRINT("Visiting And\n");
    return AI.getOperand(0);
  }
  // When facing %flag.addr, find below:
  // store %flag, ptr %flag.addr, align 4
  Value *visitAllocaInst(AllocaInst &AI) {
    DEBUG_PRINT("Visiting Alloca\n");
    for (User *U : AI.users()) {
      if (isa<StoreInst>(U)) {
        DEBUG_PRINT("is Store\n");
        return U;
      }
    }
    return nullptr;
  }
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
      DEBUG_PRINT("getOrigin: ");
      if (isa<Function>(val)) {
        DEBUG_PRINT(val->getName() << "\n");
      } else {
        DEBUG_PRINT(*val << "\n");
      }

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

    DEBUG_PRINT("** Too deep: " << val << "\n");
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
    DEBUG_PRINT("PtrOp: " << *PtrOp << "\n");
    return getVarName(*PtrOp);
  }

  /*
   * Get variable name
   */
  StringRef getVarName(Value &V) {
    StringRef Name = getOrigin(V)->getName();
    if (Name.empty())
      Name = "Unnamed Condition";
    /* StringRef StructName = getStructName(V); */
    /* if (!StructName.empty()) */
    /*     Name = Twine(StructName) + "." + Twine(Name); */
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
  bool findIfCond_cmp(BranchInst &BrI, CmpInst &CmpI, BasicBlock &DestBB,
                      std::vector<Condition *> &conds) {
    StringRef name;
    Value *val;
    CondType type;

    Value *CmpOp = CmpI.getOperand(0);
    if (!CmpOp)
      return false;
    /* DEBUG_PRINT(*CmpI->getParent() << "\n"); */

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
      type = isBranchTrue(BrI, DestBB) ? NLLFLS : NLLTRU;
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
      type = isBranchTrue(BrI, DestBB) ? CMPTRU : CMPFLS;
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
      type = isBranchTrue(BrI, DestBB) ? CALTRU : CALFLS;
      conds.push_back(new Condition(name, val, type));
      return true;
    }

    // CmpOp: %1 = load i32, i32* %flag.addr, align 4
    // TODO: Try on some examples
    if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
      DEBUG_PRINT("LoadI as CmpOp: " << *LoadI << "\n");
      name = getVarName(*LoadI);
      val = CmpI.getOperand(1);

      type = isBranchTrue(BrI, DestBB) ? CMPTRU : CMPFLS;
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
  bool findIfCond_call(BranchInst &BrI, CallInst &CallI, BasicBlock &DestBB,
                       std::vector<Condition *> &conds) {
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
  bool findIfCond(BranchInst &BrI, BasicBlock &DestBB,
                  std::vector<Condition *> &conds) {
    StringRef name;
    Value *val;

    Value *IfCond = BrI.getCondition();
    if (!IfCond)
      return false;

    // And condition: if (flag & 2) {}
    if (isa<CmpInst>(IfCond)) {
      return findIfCond_cmp(BrI, cast<CmpInst>(*IfCond), DestBB, conds);
    }
    // Call condition: if (!func()) {}
    if (isa<CallInst>(IfCond)) {
      return findIfCond_call(BrI, cast<CallInst>(*IfCond), DestBB, conds);
    }
    DEBUG_PRINT("** Unexpected as IfCond: " << *IfCond << "\n");
    return false;
  }

  /* Get name & value of switch condition
   * Returns: (StringRef, Value*)
     - switch (flag) {}     // name:of flag, val:of flag
   */
  bool findSwCond(SwitchInst &SwI, std::vector<Condition *> &conds) {
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
   * Returns: BasicBlock*
   */
  BasicBlock *getCondBB(BasicBlock &BB) {
    /* DEBUG_PRINT("Search preds of: " << *BB << "\n"); */
    for (auto *PredBB : predecessors(&BB)) {
      Instruction *TI = PredBB->getTerminator();
      if (isa<BranchInst>(TI)) {
        // Branch sometimes has only one successor
        // e.g. br label %if.end
        if (TI->getNumSuccessors() != 2)
          continue;
        return PredBB;
      }
      if (isa<SwitchInst>(TI)) {
        return PredBB;
      }
      DEBUG_PRINT(
          "**************PredBB terminator is not a branch or switch\n");
      DEBUG_PRINT("**************PredBB: " << *PredBB << "\n");
    }
    return nullptr;
  }

  bool findConditions(BasicBlock &CondBB, BasicBlock &DestBB,
                      std::vector<Condition *> &conds) {
    DEBUG_PRINT("Trying to get conditions\n");

    // Get Condition
    /*
     * if (flag & 2) {}     // name:of flag, val:val
     * switch (flag) {}     // name:of flag, val:of flag
     */
    if (auto *BrI = dyn_cast<BranchInst>(CondBB.getTerminator())) {
      DEBUG_PRINT("Pred has BranchInst\n");
      return findIfCond(*BrI, DestBB, conds);
    } else if (auto *SwI = dyn_cast<SwitchInst>(CondBB.getTerminator())) {
      DEBUG_PRINT("Pred has SwitchInst\n");
      return findSwCond(*SwI, conds);
    } else {
      DEBUG_PRINT("* CondBB terminator is not a branch or switch\n");
      return false;
    }
  }

  // Search from bottom to top (entry block)
  void findAllConditions(BasicBlock &ErrBB, std::vector<Condition *> &conds) {
    BasicBlock *BB = &ErrBB;
    for (int i = 0; i < MAX_TRACE_DEPTH; i++) {

      // Get Condition BB
      BasicBlock *CondBB = getCondBB(*BB);
      if (!CondBB) {
        DEBUG_PRINT("*OMGOMGOMGOMG No CondBB in preds\n");
        break;
      }

      // Get Condition
      // TODO: Should this return bool?
      if (!findConditions(*CondBB, *BB, conds)) {
        DEBUG_PRINT("* findCond has failed.\n");
        break;
      }

      // Update BB
      // NOTE: We need CondBB, not only its terminator
      BB = CondBB;

      // The loop reaches to the end?
      if (CondBB == &CondBB->getParent()->getEntryBlock()) {
        DEBUG_PRINT("* Reached to the entry\n");
        break;
      }
      if (i == MAX_TRACE_DEPTH - 1) {
        DEBUG_PRINT("* Reached to the max depth\n");
        break;
      }
    }
  }

  /*
   * Delete all conditions
   * Call this when you continue to next ErrBB
   */
  void deleteAllCond(std::vector<Condition *> &conds) {
    while (!conds.empty()) {
      delete conds.back();
      conds.pop_back();
    }
  }

  // Debug info
  // NOTE: need clang flag "-g"
  void getDebugInfo(User *U, Function &F, std::vector<Condition *> &conds) {
    LLVMContext &Ctx = U->getContext();
    StringRef filename = F.getParent()->getSourceFileName();
    const DebugLoc &Loc = dyn_cast<Instruction>(U)->getDebugLoc();
    unsigned line = Loc->getLine();
    DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
    Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
    // The analyzing function
    conds.push_back(new Condition(
        F.getName(), dyn_cast<StoreInst>(U)->getValueOperand(), CALFLS));
    // File name and line number
    conds.push_back(new Condition(filename, lineVal, DBINFO));
    // Hello
    conds.push_back(new Condition("", NULL, HELLOO));
  }

  void insertLoggers(BasicBlock *ErrBB, Function &F,
                     std::vector<Condition *> &conds) {
    DEBUG_PRINT("...Inserting log...\n");

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

      // Check if the Value is ConstantPointerNull
      if (cond->Val && isa<ConstantPointerNull>(cond->Val)) {
        DEBUG_PRINT("Val is ConstantPointerNull\n");
        if (cond->Type == CMPTRU)
          cond->Type = NLLTRU; // maybe never reached
        else if (cond->Type == CMPFLS)
          cond->Type = NLLFLS; // maybe never reached
        else
          DEBUG_PRINT("** Unexpected type:" << cond->Type << "\n");
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
    }
  }

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

    return RetVal;
  }

  /* Get error-thrower BB "ErrBB"
     store i32 -13，ptr %1, align 4
   * Returns: BasicBlock*
   */
  BasicBlock *getErrBB(User *U) {
    StoreInst *SI = dyn_cast<StoreInst>(U);
    if (!SI)
      return nullptr;

    // Storing -EACCES?
    Value *ValOp = SI->getValueOperand();
    ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
    if (!CI)
      return nullptr;

    if (CI->getSExtValue() != -EACCES)
      return nullptr;

    // TODO: Check ALL error codes
    /* if (CI->getSExtValue() >= 0) */
    /*     return nullptr; */

    // Error-thrower BB (BB of store -EACCES)
    BasicBlock *ErrBB = SI->getParent();

    return ErrBB;
  }

  /*
   *****************
   * main function *
   *****************
   */
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    bool modified = false;
    for (auto &F : M.functions()) {
      DEBUG_PRINT("FUNCTION: " << F.getName() << "\n");
      if (F.getName() == "lookup_open")
        DEBUG_PRINT(F);

      // Skip
      if (F.getName() == LOGGER)
        continue;

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

        // Get Error-thrower BB "ErrBB"
        /* store -13, ptr %1, align 4 */
        BasicBlock *ErrBB = getErrBB(U);
        if (!ErrBB)
          continue;

        DEBUG_PRINT("\n///////////////////////////////////////\n");
        DEBUG_PRINT(F.getName() << " has 'return -EACCES'\n");
        DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");
        /* DEBUG_PRINT(*(dyn_cast<StoreInst>(U)->getValueOperand()) <<
         * "!!\n"); */

        // Prepare Array of Condition
        std::vector<Condition *> conds;

        // Backtrace to find If/Switch Statement BB
        findAllConditions(*ErrBB, conds);
        if (conds.empty()) {
          DEBUG_PRINT("** conds is empty\n");
          continue;
        }

        getDebugInfo(U, F, conds);

        // Insert loggers
        insertLoggers(ErrBB, F, conds);

        if (conds.empty()) {
          DEBUG_PRINT("~~~ Inserted all logs ~~~\n");
        } else {
          DEBUG_PRINT("** Failed to insert all logs\n");
          deleteAllCond(conds);
        }

        // Declare the modification
        // NOTE: always modified, because we insert at least debug info.
        modified = true;
      }
    }
    if (modified)
      return PreservedAnalyses::none();
    return PreservedAnalyses::all();
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
