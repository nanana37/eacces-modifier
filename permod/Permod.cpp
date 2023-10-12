#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

#define DEBUG 1

using namespace llvm;

namespace {

struct PermodPass : public PassInfoMixin<PermodPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        for (auto &F : M.functions()) {
            #ifdef DEBUG
            errs() << "Function: " << F.getName() << "\n";
            #endif

            for (auto &BB : F) {
                IRBuilder<> builder(&BB);
                ReturnInst *RI = dyn_cast<ReturnInst>(BB.getTerminator());

                if (!RI) {
                    #ifdef DEBUG
                    errs() << "No return instruction found\n";
                    #endif

                    continue;
                }

                if (ConstantInt *CI = dyn_cast<ConstantInt>(RI->getReturnValue())) {
                    #ifdef DEBUG
                    errs() << "Return value: " << CI->getSExtValue() << "\n";
                    #endif
                    switch (CI->getSExtValue()) {
                    case -EACCES:
                        #ifdef DEBUG
                        errs() << "Found EACCES\n";
                        #endif
                        // Insert a call to printf before the return instruction
                        /* builder.SetInsertPoint(RI); */
                        /* builder.CreateCall( */
                        /*         Printf, */ 
                        /*         ) */

                        return PreservedAnalyses::none();
                    default:
                        break;
                    }
                }
            }
        }
        return PreservedAnalyses::all();
    };
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Permod pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(PermodPass());
                });
        }
    };
}
