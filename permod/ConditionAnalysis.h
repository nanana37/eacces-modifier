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
class ConditionAnalysis {
private:
  /* Variables */
  // Prepare Array of Condition
  typedef std::vector<Condition *> CondVec;
  CondVec preConds;
  CondVec postConds;
  std::unordered_set<BasicBlock *> visitedBBs;

  // ****************************************************************************
  //                               Utility
  // ****************************************************************************
  Value *getOrigin(Value &V);
  StringRef getStructName(Value &V);
  StringRef getVarName(Value &V);
  void prepareFormat(Value *format[], IRBuilder<> &builder, LLVMContext &Ctx);

  /*
   * ****************************************************************************
   *                       Anlyzing Conditions
   * ****************************************************************************
   */
  /*
   * Delete all conditions
   * Call this when you continue to next ErrBB
   */
  void deleteAllConds() {
    preConds.clear();
    postConds.clear();
  }

  bool isEmpty() { return preConds.empty() && postConds.empty(); }
  bool isBranchTrue(BranchInst &BrI, BasicBlock &DestBB) {
    if (BrI.getSuccessor(0) == &DestBB)
      return true;
    return false;
  }
  bool isDead(BasicBlock &BB);

  Condition *findIfCond_cmp(CondVec &conds, BranchInst &BrI, CmpInst &CmpI,
                            BasicBlock &DestBB);
  Condition *findIfCond_call(CondVec &conds, BranchInst &BrI, CallInst &CallI,
                             BasicBlock &DestBB);
  Condition *findIfCond(CondVec &conds, BranchInst &BrI, BasicBlock &DestBB);
  Condition *findSwCond(CondVec &conds, SwitchInst &SwI);
  Condition *findConditions(CondVec &conds, BasicBlock &CondBB,
                            BasicBlock &DestBB);
  void findPredConditions(BasicBlock &ErrBB, int depth = 0);
  void findPostConditions(BasicBlock &ErrBB, int depth = 0);

  void getDebugInfo(Instruction &I, Function &F);
  bool insertLoggers(BasicBlock &ErrBB, Function &F);

public:
  bool main(BasicBlock &ErrBB, Function &F, Instruction &I);
};
} // namespace permod
