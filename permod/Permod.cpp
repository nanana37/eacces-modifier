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

            // This assumes that there is only one Term BB
            bool IsReturnFound = false;
            Value *RetVal = nullptr;

            // Search for Terminator BB (Contains return instruction)
            for (auto &BB : F) {
                // Get the Terminator Instruction
                Instruction *TI = BB.getTerminator();
                if (!TI) continue;

                #ifdef DEBUG
                errs() << "Found Terminator Instruction...\n";
                errs() << *TI << "\n";
                #endif

                // Check if this BB is a Terminator BB
                ReturnInst *RI = dyn_cast<ReturnInst>(TI);
                if (!RI) continue;

                #ifdef DEBUG
                errs() << "Found Return Instruction!\n";
                errs() << *RI << "\n";
                #endif

                // This assumes that there is only one Term BB
                IsReturnFound = true;
                RetVal = RI->getReturnValue();
                errs() << "Return Value: " << *RetVal << "\n";
                break;

            }

            // This assumes that there is only one Term BB
            if (!IsReturnFound) continue;

            // Use-Def chain for the found return value
            for (User *U : RetVal->users()) {
                Instruction *UI = dyn_cast<Instruction>(U);
                if (!UI) continue;

                #ifdef DEBUG
                errs() << "Found User Instruction!\n";
                errs() << *UI << "\n";
                #endif
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
