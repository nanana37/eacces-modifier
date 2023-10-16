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

/* 
 * Find 'return -EACCES'
    * The statement turns into:
    Error-catcher BB:
        store i32 -13，ptr %1, align 4
    Terminator BB:
        %2 = load i32, ptr %1, align 4
        ret i32 %2
 */

struct PermodPass : public PassInfoMixin<PermodPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        for (auto &F : M.functions()) {
            #ifdef DEBUG
            errs() << "Function: " << F.getName() << "\n";
            #endif

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
                errs() << "Found BB of Return\n";
                errs() << *RI << "\n";
                #endif

                // Get the return value
                Value *RetVal = RI->getReturnValue();
                errs() << "Return Value: " << *RetVal << "\n";
                // Get def of the return value
                LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
                if (!DefLI) continue;
                RetVal = DefLI->getPointerOperand();
                errs() << "Def of Return Value: " << *RetVal << "\n";

                // Use-Def chain for the found return value
                for (User *U : RetVal->users()) {
                    #ifdef DEBUG
                    errs() << "Checking User:\n" << *U << "\n";
                    #endif
                    // Store instruction stores '-EACCES' (-13)
                    StoreInst *SI = dyn_cast<StoreInst>(U);
                    if (!SI) continue;

                    Value *ValOp = SI->getValueOperand();
                    ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
                    if (!CI) continue;
                    if (CI->getSExtValue() != -EACCES) continue;
                    errs() << "** Found -EACCES! **\n\n";
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
