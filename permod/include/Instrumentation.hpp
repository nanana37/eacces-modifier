
#pragma once

#include "llvm/IR/IRBuilder.h"

#include "Condition.hpp"
#include "macro.h"

#ifdef DEBUG
extern const char *condTypeStr[];
#endif // DEBUG

#ifndef TEST
#define LOGGR_FUNC "_printk"
#else
#define LOGGR_FUNC "printf"
#endif // TEST

#define BUFFR_FUNC "buffer_cond"
#define FLUSH_FUNC "flush_cond"

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
  bool insertBufferFunc(CondStack &Conds, BasicBlock &TheBB,
                        long long &cond_num);
  bool insertFlushFunc(CondStack &Conds, BasicBlock &TheBB);
  bool insertLoggers(CondStack &Conds, BasicBlock &TheBB);
};