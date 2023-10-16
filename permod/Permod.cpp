#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

#define DEBUG
#ifdef DEBUG
    #define DEBUG_PRINT(x) do { errs() << x; } while (0)
#endif

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
            DEBUG_PRINT("Function: " << F.getName() << "\n");

            // Search for Terminator BB (Contains return instruction)
            for (auto &BB : F) {
                DEBUG_PRINT("Basic Block:" << BB << "\n");

                // Get the Terminator Instruction
                Instruction *TI = BB.getTerminator();
                if (!TI) continue;
                DEBUG_PRINT("Terminator Instruction: " << *TI << "\n");

                // Check if this BB is a Terminator BB
                ReturnInst *RI = dyn_cast<ReturnInst>(TI);
                if (!RI) continue;
                DEBUG_PRINT("~Found BB of Return\n");

                // Use of return value
                Value *RetVal = RI->getReturnValue();
                DEBUG_PRINT("Return Value: " << *RetVal << "\n");

                // Def of return value
                LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
                if (!DefLI) continue;
                RetVal = DefLI->getPointerOperand();
                DEBUG_PRINT("Def of Return Value: " << *RetVal << "\n");

                // Def-Use chain for the found return value
                for (User *U : RetVal->users()) {
                    DEBUG_PRINT("Checking User: " << *U << "\n");

                    // Store instruction stores '-EACCES' (-13)
                    StoreInst *SI = dyn_cast<StoreInst>(U);
                    if (!SI) continue;

                    Value *ValOp = SI->getValueOperand();
                    ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
                    if (!CI) continue;
                    if (CI->getSExtValue() != -EACCES) continue;
                    DEBUG_PRINT("'return -EACCES' found!\n");
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
