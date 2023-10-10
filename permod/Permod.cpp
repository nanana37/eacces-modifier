#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

using namespace llvm;

namespace {

struct PermodPass : public PassInfoMixin<PermodPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        int num = 0;
        for (auto &F : M.functions()) {
            for (auto &B : F) {
                IRBuilder<> builder(&B);
                ReturnInst *RI = dyn_cast<ReturnInst>(B.getTerminator());

                if (ConstantInt *CI = dyn_cast<ConstantInt>(RI->getReturnValue())) {
                    switch (CI->getSExtValue()) {
                    case -EACCES:
                        // Print with number
                        errs() << "Found -EACCES: " << ++num << "\n";
                        break;
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
