// #define TEST

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/DebugLoc.h"

#include "ConditionAnalysis.h"
#include "ErrBBfinder.h"
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

  bool analyzeStorePtr(StoreInst &SI, Function &F) {
    DEBUG_PRINT2("\n==AnalysisForStorePtr==\n");
    DEBUG_VALUE(&SI);

    bool modified = false;

    Value *ErrVal = EBF.getErrValue(SI);
    if (!ErrVal)
      return false;

    DEBUG_VALUE(ErrVal);

    for (User *U : ErrVal->users()) {
      StoreInst *SI = dyn_cast<StoreInst>(U);
      if (!SI)
        continue;
      BasicBlock *ErrBB = EBF.getErrBB(*SI);
      if (!ErrBB)
        continue;
      DEBUG_PRINT("\n///////////////////////////////////////\n");
      DEBUG_PRINT(F.getName() << " has 'return -ERRNO'\n");
      DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");

      // TODO: This should be class; only main() should be public
      struct ConditionAnalysis ConditionAnalysis {};

      ConditionAnalysis.main(*ErrBB, F, *SI);
    }
    return modified;
  }

  bool analyzeStoreInt32(StoreInst &SI, Function &F) {
    DEBUG_PRINT2("\n==AnalysisForStoreInt32==\n");
    DEBUG_VALUE(&SI);

    bool modified = false;

    // Get Error-thrower BB "ErrBB"
    /* store -13, ptr %1, align 4 */
    BasicBlock *ErrBB = EBF.getErrBB(SI);
    if (!ErrBB)
      return false;

    DEBUG_PRINT("\n///////////////////////////////////////\n");
    DEBUG_PRINT(F.getName() << " has 'return -ERRNO'\n");
    DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");

    struct ConditionAnalysis ConditionAnalysis {};

    ConditionAnalysis.main(*ErrBB, F, SI);

    return modified;
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

      // FIXME: this function cause crash
      if (F.getName() == "profile_transition") {
        DEBUG_PRINT("--- Skip profile_transition\n");
        // DEBUG_PRINT(F);
        continue;
      } else {
        // continue;
      }

      // Skip
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

      Value *RetVal = EBF.getReturnValue(F);
      if (!RetVal)
        continue;

      for (User *U : RetVal->users()) {
        StoreInst *SI = dyn_cast<StoreInst>(U);
        if (!SI)
          continue;

        // Analyze conditions & insert loggers
        if (EBF.isStorePtr(*SI)) {
          modified |= analyzeStorePtr(*SI, F);
        } else {
          // TODO: Is this really "else"?
          modified |= analyzeStoreInt32(*SI, F);
        }
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
