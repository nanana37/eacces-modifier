#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/DebugLoc.h"

#include "ConditionAnalysis.h"
#include "ErrBBFinder.h"
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
      StoreInst *SI = dyn_cast<StoreInst>(&V);
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
      if (F.getName() == "profile_transition") {
        DEBUG_PRINT("--- Skip profile_transition\n");
        // DEBUG_PRINT(F);
        continue;
      } else {
        // continue;
      }
      if (F.isDeclaration()) {
        DEBUG_PRINT2("--- Skip Declaration\n");
        continue;
      }
      if (F.getName().startswith("llvm")) {
        DEBUG_PRINT2("--- Skip llvm\n");
        continue;
      }
      if (F.getName() == LOGGER) {
        DEBUG_PRINT2("--- Skip Logger\n");
        continue;
      }

      /*
       * Get return value of the function.
       * There is only one return (& return value) in a function, thanks to
       * mergereturn optimization.
       */
      Value *RetVal = EBF.getReturnValue(F);
      if (!RetVal)
        continue;

      /* The function returns error value. */
      if (isBeingErrno(*RetVal))
        continue;

      DEBUG_PRINT("\n///////////////////////////////////////\n");
      DEBUG_PRINT(F.getName() << " has 'return -ERRNO'\n");

      /* Insert logger just before terminator of every BB */
      for (BasicBlock &BB : F) {
        if (BB.getTerminator()->getNumSuccessors() <= 1) {
          DEBUG_PRINT2("Skip BB: " << BB.getName() << "\n");
          DEBUG_PRINT2(BB);
          continue;
        }
        DEBUG_PRINT2("Instrumenting BB: " << BB.getName() << "\n");
        struct ConditionAnalysis CA(&BB);
        // FIXME: the succesor should be determined dynamically
        CA.findConditions(BB, *BB.getTerminator()->getSuccessor(0));
        CA.insertLoggers(BB, F);
      }
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
