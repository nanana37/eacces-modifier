#ifndef CONDITION_ANALYSIS_H
#define CONDITION_ANALYSIS_H

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"

#include <unordered_set>

#include "OriginFinder.h"
#include "debug.h"

#ifdef TEST
#define LOGGER "printf"
#else
#define LOGGER "_printk"
#endif

using namespace llvm;

namespace permod {
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
    DEBUG_PRINT2("Name: " << Name << "\n");
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
    CMP_GT,
    CMP_GE,
    CMP_LT,
    CMP_LE,
    NLLTRU,
    NLLFLS,
    CALTRU,
    CALFLS,
    ANDTRU,
    ANDFLS,
    SWITCH,
    DBINFO,
    HELLOO,
    _OPEN_,
    _CLSE_,
    NUM_OF_CONDTYPE
  };

#ifdef DEBUG
  const char *condTypeStr[NUM_OF_CONDTYPE] = {
      "CMPTRU", "CMPFLS", "CMP_GT", "CMP_GE", "CMP_LT", "CMP_LE",
      "NLLTRU", "NLLFLS", "CALTRU", "CALFLS", "ANDTRU", "ANDFLS",
      "SWITCH", "DBINFO", "HELLOO", "_OPEN_", "_CLSE_"};
#endif // DEBUG

  class Condition {
  public:
    StringRef Name;
    Value *Val;
    CondType Type;

    Condition(StringRef name, Value *val, CondType type)
        : Name(name), Val(val), Type(type) {}

  private:
    void setType(CmpInst &CmpI, bool isBranchTrue) {
      switch (CmpI.getPredicate()) {
      case CmpInst::Predicate::ICMP_EQ:
        Type = isBranchTrue ? CMPTRU : CMPFLS;
        break;
      case CmpInst::Predicate::ICMP_NE:
        Type = isBranchTrue ? CMPFLS : CMPTRU;
        break;
      case CmpInst::Predicate::ICMP_UGT:
      case CmpInst::Predicate::ICMP_SGT:
        Type = isBranchTrue ? CMP_GT : CMP_LE;
        break;
      case CmpInst::Predicate::ICMP_UGE:
      case CmpInst::Predicate::ICMP_SGE:
        Type = isBranchTrue ? CMP_GE : CMP_LT;
        break;
      case CmpInst::Predicate::ICMP_ULT:
      case CmpInst::Predicate::ICMP_SLT:
        Type = isBranchTrue ? CMP_LT : CMP_GE;
        DEBUG_PRINT(isBranchTrue);
        break;
      case CmpInst::Predicate::ICMP_ULE:
      case CmpInst::Predicate::ICMP_SLE:
        Type = isBranchTrue ? CMP_LE : CMP_GT;
        break;
      default:
        DEBUG_PRINT("******* Sorry, Unexpected CmpInst::Predicate\n");
        DEBUG_PRINT(CmpI);
        Type = DBINFO;
        break;
      }
    }

  public:
    Condition(StringRef name, Value *val, CmpInst &CmpI, bool isBranchTrue)
        : Name(name), Val(val) {
      setType(CmpI, isBranchTrue);
    }

    Condition(StringRef name, Value *val, CondType type, CmpInst &CmpI,
              bool isBranchTrue)
        : Name(name), Val(val), Type(type) {
      setType(CmpI, isBranchTrue);
    }
  };

  /*
   * Prepare format string
   */
  void prepareFormat(Value *format[], IRBuilder<> &builder, LLVMContext &Ctx) {
    StringRef formatStr[NUM_OF_CONDTYPE];
    formatStr[CMPTRU] = "[Permod] %s == %d\n";
    formatStr[CMPFLS] = "[Permod] %s != %d\n";
    formatStr[CMP_GT] = "[Permod] %s > %d\n";
    formatStr[CMP_GE] = "[Permod] %s >= %d\n";
    formatStr[CMP_LT] = "[Permod] %s < %d\n";
    formatStr[CMP_LE] = "[Permod] %s <= %d\n";
    formatStr[NLLTRU] = "[Permod] %s == null\n";
    formatStr[NLLFLS] = "[Permod] %s != null\n";
    formatStr[CALTRU] = "[Permod] %s() didn't return %d\n";
    formatStr[CALFLS] = "[Permod] %s() returned %d\n";
    formatStr[ANDTRU] = "[Permod] %s & %d > 0\n";
    formatStr[ANDFLS] = "[Permod] %s & %d == 0\n";
    formatStr[SWITCH] = "[Permod] %s == %d (switch)\n";
    formatStr[DBINFO] = "[Permod] %s: %d\n";
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
      val = CmpOp2;
      type = (isBranchTrue(BrI, DestBB) != CmpI.isFalseWhenEqual()) ? NLLTRU
                                                                    : NLLFLS;
      conds.push_back(new Condition(name, val, type));
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
        conds.push_back(new Condition(name, val, type));
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
      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CALTRU
                                                                    : CALFLS;
      conds.push_back(
          new Condition(name, val, CmpI, isBranchTrue(BrI, DestBB)));
      return true;
    }

    // CmpOp: %1 = load i32, i32* %flag.addr, align 4
    // TODO: Try on some examples
    if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
      name = getVarName(*LoadI);
      val = CmpI.getOperand(1);

      type = (isBranchTrue(BrI, DestBB) == CmpI.isFalseWhenEqual()) ? CMPTRU
                                                                    : CMPFLS;
      conds.push_back(
          new Condition(name, val, CmpI, isBranchTrue(BrI, DestBB)));

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

  bool findConditions(BasicBlock &CondBB, BasicBlock &DestBB) {
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
  void findAllConditions(BasicBlock &ErrBB, int depth = 0) {
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
    // The analyzing function
    conds.push_back(new Condition(
        F.getName(), dyn_cast<StoreInst>(&SI)->getValueOperand(), CALFLS));

    LLVMContext &Ctx = SI.getContext();
    StringRef filename = F.getParent()->getSourceFileName();
    const DebugLoc &Loc = dyn_cast<Instruction>(&SI)->getDebugLoc();
    unsigned line = Loc->getLine();
    DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
    Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
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

      args.push_back(format[cond->Type]);
      DEBUG_PRINT(condTypeStr[cond->Type]);

      switch (cond->Type) {
      case HELLOO:
      case _OPEN_:
      case _CLSE_:
        DEBUG_PRINT("\n");
        break;
      default:
        DEBUG_PRINT(" " << cond->Name << ": " << *cond->Val << "\n");
        args.push_back(builder.CreateGlobalStringPtr(cond->Name));
        args.push_back(cond->Val);
        break;
      }

      builder.CreateCall(logFunc, args);
      args.clear();
      delete cond;
      modified = true;
    }

    return modified;
  }

  bool isEmpty() { return conds.empty(); }
};
} // namespace permod
#endif // CONDITION_ANALYSIS_H
