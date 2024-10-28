#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/DebugLoc.h"

#include "ConditionAnalysis.hpp"
#include "ErrBBFinder.hpp"
#include "Instrumentation.hpp"
#include "debug.h"

using namespace llvm;

namespace permod {

/*
 * Find 'return -ERRNO'
    * The statement turns into:
    Error-thrower BB:
        store i32 -13，ptr %1, align 4
    Terminator BB:
        %2 = load i32, ptr %1, align 4
        ret i32 %2
 */
struct PermodPass : public PassInfoMixin<PermodPass> {

  ErrBBFinder EBF;

  /*
   * Check whether @V (typically retval) will be assigned errno.
   */
  bool isBeingErrno(Value &V) {
    /* When the function has a single return statement which returns errno. */
    if (EBF.isErrno(V))
      return true;

    /* Use-def chain to find the assignment of errno to the return value. */
    for (User *U : V.users()) {
      StoreInst *SI = dyn_cast<StoreInst>(U);
      if (!SI)
        continue;

      /* `return ERR_PTR(errno);` is converted into `store ptr` */
      if (EBF.isStorePtr(*SI)) {
        Value *ErrVal = EBF.getErrValue(*SI);
        if (!ErrVal)
          continue;
        for (User *UP : ErrVal->users()) {
          StoreInst *SIP = dyn_cast<StoreInst>(UP);
          if (!SIP)
            continue;
          if (EBF.getErrBB(*SIP))
            return true;
        }
      } else {
        /* When one of multiple return statements returns errno. */
        if (EBF.getErrBB(*SI))
          return true;
      }
    }

    return false;
  }

  /*
   *****************
   * main function *
   *****************
   */
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    DEBUG_PRINT("\n@@@ PermodPass @@@\n");
    DEBUG_PRINT("Module: " << M.getName() << "\n");
    bool modified = false;
    for (auto &F : M.functions()) {
      DEBUG_PRINT2("\n-FUNCTION: " << F.getName() << "\n");

      /* Skip some functions */
      // FIXME: this function cause crash
#ifdef DEBUG_FIXME
      if (F.getName() != "may_open") {
        continue;
      }
#endif
      if (F.isDeclaration()) {
        DEBUG_PRINT2("--- Skip Declaration\n");
        continue;
      }
      if (F.getName().startswith("llvm")) {
        DEBUG_PRINT2("--- Skip llvm\n");
        continue;
      }
      if (F.getName() == LOGGR_FUNC || F.getName() == BUFFR_FUNC ||
          F.getName() == FLUSH_FUNC) {
        DEBUG_PRINT2("--- Skip permod API\n");
        continue;
      }

      /* Find return statement of the function */
      ReturnInst *RetI = EBF.findRetInst(F);
      if (!RetI) {
        DEBUG_PRINT2("*** No return\n");
        continue;
      }

      /* Insert loggers into function which returns error value. */
      Value *RetV = EBF.findRetValue(*RetI);
      if (!RetV) {
        DEBUG_PRINT2("*** No return value\n");
        continue;
      }

      /* Check whether the return value will be assigned errno. */
      if (!isBeingErrno(*RetV)) {
        DEBUG_PRINT2("*** No errno\n");
        continue;
      }

      DEBUG_PRINT("\n///////////////////////////////////////\n");
      DEBUG_PRINT(F.getName() << " has 'return -ERRNO'\n");

      long long cond_num = 0;

      /* Insert logger just before terminator of every BB */
      for (BasicBlock &BB : F) {
        if (BB.getTerminator()->getNumSuccessors() <= 1) {
          DEBUG_PRINT2("Skip BB: " << BB.getName() << "\n");
          DEBUG_PRINT2(BB);
          continue;
        }
        DEBUG_PRINT2("Instrumenting BB: " << BB.getName() << "\n");
        CondStack Conds;
        // NOTE: @DestBB is used to determine which path is true.
        ConditionAnalysis::findConditions(Conds, BB,
                                          *BB.getTerminator()->getSuccessor(0));
        class Instrumentation Ins(&BB);
        if (Ins.insertBufferFunc(Conds, BB, cond_num)) {
          modified = true;
          cond_num++;
        }
      }

      CondStack RetConds;
      BasicBlock *RetBB = RetI->getParent();
      ConditionAnalysis::setRetCond(RetConds, *RetBB);
      class Instrumentation Ins(RetBB);
      modified |= Ins.insertFlushFunc(RetConds, *RetBB);
    }

    if (modified) {
      DEBUG_PRINT("Modified\n");
      return PreservedAnalyses::none();
    } else {
      DEBUG_PRINT2("Not Modified\n");
      return PreservedAnalyses::all();
    }
  };
}; // namespace

} // namespace permod

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Permod pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(permod::PermodPass());
                });
          }};
}
