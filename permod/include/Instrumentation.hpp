
#pragma once

#include "llvm/IR/IRBuilder.h"

#include "Condition.hpp"
#ifdef DEBUG
extern const char *condTypeStr[];
#endif // DEBUG

using namespace llvm;
using namespace permod;

class Instrumentation {
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
  Instrumentation(BasicBlock *RetBB)
      : TargetFunc(RetBB->getParent()), RetBB(RetBB), Builder(RetBB),
        Ctx(RetBB->getContext()) {
    prepFormat();
    prepLogger();
  }

  /* Instrumentation */
  bool insertBufferFunc(CondStack &Conds, BasicBlock &TheBB);
  bool insertFlushFunc(CondStack &Conds, BasicBlock &TheBB);
  bool insertLoggers(CondStack &Conds, BasicBlock &TheBB);
};
