//===- permod/ErrBBFinder.h - Definition of the ErrBBFinder -----*-C++-*-===//
/*
 * ****************************************************************************
 *                                Find 'return -ERRNO'
 * ****************************************************************************
 */

#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace permod {

struct ErrBBFinder {
  Value *getReturnValue(Function &F);
  Value *findReturnValue(BasicBlock &BB);
  bool isErrno(Value &V);
  Value *getErrno(Value &V);
  bool isStoreErr(StoreInst &SI);
  BasicBlock *getErrBB(StoreInst &SI);
  bool isStorePtr(StoreInst &SI);
  Value *getErrValue(StoreInst &SI);
};

} // namespace permod
