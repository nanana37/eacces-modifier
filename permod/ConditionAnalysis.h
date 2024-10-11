//===- permod/ConditionAnalysis.h - Definition of the ConditionAnalysis -===//
//
// Analyze conditions that lead to an error
//

#pragma once

#include "llvm/IR/IRBuilder.h"

#include <unordered_set>

#include "Condition.h"
#ifdef DEBUG
extern const char *condTypeStr[];
#endif // DEBUG

using namespace llvm;

namespace permod {
struct ConditionAnalysis {
private:
  /* Analysis Result */
  std::vector<Condition *> Conds;
  std::unordered_set<BasicBlock *> VistedBBs;

  /* Analysis Target */
  Function *TargetFunc;
  BasicBlock *RetBB;

  /* IRBuilder */
  IRBuilder<> Builder;
  LLVMContext &Ctx;

  /* Logger */
  Value *Format[NUM_OF_CONDTYPE];
  FunctionCallee LogFunc;

  /* Constructor methods */
  void prepFormat();
  void prepLogger();

public:
  /* Constructor */
  ConditionAnalysis(BasicBlock *RetBB)
      : TargetFunc(RetBB->getParent()), RetBB(RetBB), Builder(RetBB),
        Ctx(RetBB->getContext()) {
    prepFormat();
    prepLogger();
  }

  // ****************************************************************************
  //                               Utility
  // ****************************************************************************
  Value *getOrigin(Value &V);
  StringRef getStructName(Value &V);
  StringRef getVarName(Value &V);

  /*
   * ****************************************************************************
   *                       Anlyzing Conditions
   * ****************************************************************************
   */

  /*
   * Delete all conditions
   * Call this when you continue to next ErrBB
   */
  void deleteAllCond() {
    while (!Conds.empty()) {
      delete Conds.back();
      Conds.pop_back();
    }
  }
  bool isEmpty() { return Conds.empty(); }

  bool isBranchTrue(BranchInst &BrI, BasicBlock &DestBB) {
    if (BrI.getSuccessor(0) == &DestBB)
      return true;
    return false;
  }
  bool findIfCond_cmp(BranchInst &BrI, CmpInst &CmpI, BasicBlock &DestBB);
  bool findIfCond_call(BranchInst &BrI, CallInst &CallI, BasicBlock &DestBB);
  bool findIfCond(BranchInst &BrI, BasicBlock &DestBB);
  bool findSwCond(SwitchInst &SwI);
  bool findConditions(BasicBlock &CondBB, BasicBlock &DestBB);
  void findAllConditions(BasicBlock &ErrBB, int depth = 0);

  /* Instrumentation */
  void setRetCond(BasicBlock &theBB);
  void getDebugInfo(Instruction &I, Function &F);
  bool insertLoggers(BasicBlock &theBB);
  bool main(BasicBlock &ErrBB, Function &F, Instruction &I);
};
} // namespace permod
