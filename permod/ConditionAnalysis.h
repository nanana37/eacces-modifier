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

  // Prepare Array of Condition
  std::vector<Condition *> conds;
  std::unordered_set<BasicBlock *> visitedBBs;

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
  bool isEmpty() { return conds.empty(); }

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

  void getDebugInfo(Instruction &I, Function &F);
  bool insertLoggers(BasicBlock &ErrBB, Function &F);
  bool main(BasicBlock &ErrBB, Function &F, Instruction &I);
};
} // namespace permod
