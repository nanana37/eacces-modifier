#pragma once

#include "debug.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace permod {

struct OriginFinder : public InstVisitor<OriginFinder, Value *> {
  Value *visitFunction(Function &F) { return nullptr; }
  // When visiting undefined by this visitor
  Value *visitInstruction(Instruction &I) { return nullptr; }

  Value *visitLoadInst(LoadInst &LI) { return LI.getPointerOperand(); }
  Value *visitStoreInst(StoreInst &SI) { return SI.getValueOperand(); }
  Value *visitZExtInst(ZExtInst &ZI) { return ZI.getOperand(0); }
  Value *visitSExtInst(SExtInst &SI) { return SI.getOperand(0); }
  // TODO: Is binary operator always and?
  Value *visitBinaryOperator(BinaryOperator &BI) { return BI.getOperand(0); }

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

} // namespace permod
