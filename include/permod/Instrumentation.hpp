
#pragma once

#include "llvm/IR/IRBuilder.h"

#include "permod/Condition.hpp"
#include "utils/macro.h"

#if defined(DEBUG)
extern const char *condTypeStr[];
#endif // DEBUG

#if defined(KERNEL_MODE)
#define LOGGR_FUNC "_printk"
#else
#define LOGGR_FUNC "printf"
#endif

#define BUFFR_FUNC "buffer_cond"
#define FLUSH_FUNC "flush_cond"

using namespace llvm;
using namespace permod;

class Instrumentation {
  /* Analysis Target */
  Function *TargetFunc;

  /* Flags */
  AllocaInst *DstFlag;
  AllocaInst *ExtFlag;

  /* IRBuilder */
  LLVMContext &Ctx;
  IRBuilder<> Builder;

  /* Logger */
  Value *Format[NUM_OF_CONDTYPE];
  FunctionCallee LogFunc;

  /* Constructor methods */
  void prepFormat();
  void prepFlags();

public:
  /* Constructor */
  Instrumentation(Function *TargetFunc)
      : TargetFunc(TargetFunc), Ctx(TargetFunc->getContext()),
        Builder(TargetFunc->getContext()) {
    prepFlags();
  }

  /* Instrumentation */
  bool insertBufferFunc(BasicBlock &TheBB, DebugInfo &DBinfo,
                        long long &cond_num);
  bool insertFlushFunc(DebugInfo &DBinfo, BasicBlock &TheBB);
};
