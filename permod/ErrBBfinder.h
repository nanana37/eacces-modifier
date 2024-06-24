#pragma once

#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "OriginFinder.h"
#include "debug.h"

using namespace llvm;

namespace permod {

struct ErrBBFinder {

  /*
   * ****************************************************************************
   *                                Find 'return -ERRNO'
   * ****************************************************************************
   */
  // NOTE: Expect as Function has only one ret inst
  Value *getReturnValue(Function &F) {
    for (auto &BB : F) {
      if (Value *RetVal = findReturnValue(BB))
        return RetVal;
    }
    return nullptr;
  }

  // NOTE: maybe to find alloca is better?
  Value *findReturnValue(BasicBlock &BB) {
    if (auto *RI = dyn_cast_or_null<ReturnInst>(BB.getTerminator())) {
      Value *RetVal = RI->getReturnValue();

      if (isa_and_nonnull<ConstantInt>(RetVal))
        return RetVal;

      if (auto *LI = dyn_cast_or_null<LoadInst>(RetVal))
        return LI->getPointerOperand();
    }
    return nullptr;
  }

  bool isErrno(Value &V) {
    if (auto *CI = dyn_cast<ConstantInt>(&V)) {
      if (CI->getSExtValue() == -EACCES)
        return true;
    }
    return false;
  }

  Value *getErrno(Value &V) {
    if (isErrno(V))
      return &V;

    if (auto *StoreI = dyn_cast<StoreInst>(&V))
      return getErrno(*StoreI->getValueOperand());

    if (auto *SI = dyn_cast<SelectInst>(&V)) {
      if (isErrno(*SI->getTrueValue()))
        return getErrno(*SI->getTrueValue());
      if (isErrno(*SI->getFalseValue()))
        return getErrno(*SI->getFalseValue());
    }
    return nullptr;
  }

  bool isStoreErr(StoreInst &SI) {
    Value *ValOp = SI.getValueOperand();
    // NOTE: return -ERRNO;
    if (isErrno(*ValOp))
      return true;
    // NOTE: return (flag & 2) ? -ERRNO : 0;
    if (auto *SI = dyn_cast<SelectInst>(ValOp)) {
      if (isErrno(*SI->getTrueValue()) || isErrno(*SI->getFalseValue()))
        return true;
    }
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
    DEBUG_PRINT2("\ngetErrValue of " << SI << "\n");

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

    DEBUG_PRINT2("getErrValue ends: " << *ErrVal << "\n");

    return ErrVal;
  }
};

} // namespace permod
