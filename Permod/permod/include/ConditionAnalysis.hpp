//===- permod/ConditionAnalysis.h - Definition of the ConditionAnalysis -===//
//
// Analyze conditions that lead to an error
//

#pragma once

#include "Condition.hpp"
#include "macro.h"
#include "llvm/IR/Instructions.h"

#ifdef DEBUG
extern const char *condTypeStr[];
#endif // DEBUG

using namespace llvm;

namespace permod {
namespace ConditionAnalysis {
// ****************************************************************************
//                               Utility
// ****************************************************************************
Value *getLatestValue(AllocaInst &AI, BasicBlock &TheBB);
Value *getOrigin(Value &V);
StringRef getStructName(GetElementPtrInst &GEPI);
StringRef getVarName(Value &V);

/*
 * ****************************************************************************
 *                       Anlyzing Conditions
 * ****************************************************************************
 */
bool findIfCond_cmp(CondStack &Conds, BranchInst &BrI, CmpInst &CmpI,
                    BasicBlock &DestBB);
bool findIfCond_call(CondStack &Conds, BranchInst &BrI, CallInst &CallI,
                     BasicBlock &DestBB);
bool findIfCond(CondStack &Conds, BranchInst &BrI, BasicBlock &DestBB);
bool findSwCond(CondStack &Conds, SwitchInst &SwI);
bool findConditions(CondStack &Conds, BasicBlock &CondBB, BasicBlock &DestBB);

void setRetCond(CondStack &Conds, BasicBlock &theBB);
void getDebugInfo(DebugInfo &DBinfo, Instruction &I, Function &F);
} // namespace ConditionAnalysis
} // namespace permod
