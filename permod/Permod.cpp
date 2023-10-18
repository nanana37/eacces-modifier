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
    Error-thrower BB:
        store i32 -13，ptr %1, align 4
    Terminator BB:
        %2 = load i32, ptr %1, align 4
        ret i32 %2
 */

struct PermodPass : public PassInfoMixin<PermodPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        bool modified = false;
        for (auto &F : M.functions()) {
            DEBUG_PRINT("Function: " << F.getName() << "\n");

            // Print arg name
            for (auto &Arg : F.args()) {
                DEBUG_PRINT("Arg: " << Arg.getName() << "\n");
            }


            // Search for Terminator BB (Contains return instruction)
            for (auto &BB : F) {
                /* DEBUG_PRINT("Basic Block:" << BB); */

                // Get the Terminator Instruction
                Instruction *TI = BB.getTerminator();
                if (!TI) continue;
                /* DEBUG_PRINT("Terminator Instruction: " << *TI << "\n"); */

                // Check if this BB is a Terminator BB
                ReturnInst *RI = dyn_cast<ReturnInst>(TI);
                if (!RI) continue;
                DEBUG_PRINT("~Found BB of Return\n");

                // Use of return value
                Value *RetVal = RI->getReturnValue();
                /* DEBUG_PRINT("Return Value: " << *RetVal << "\n"); */

                // Def of return value
                LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
                if (!DefLI) continue;
                RetVal = DefLI->getPointerOperand();
                /* DEBUG_PRINT("Def of Return Value: " << *RetVal << "\n"); */

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
                    /* DEBUG_PRINT("'return -EACCES' found!\n"); */

                    // Where we found the 'return -EACCES'
                    BasicBlock *ErrBB = SI->getParent();
                    DEBUG_PRINT("-EACCES is stored by: " << *ErrBB << "\n");

                    // Find the predecessor of the Error-thrower BB
                    BasicBlock *PredBB = ErrBB->getSinglePredecessor();
                    if (!PredBB) continue;
                    DEBUG_PRINT("Predecessor of Error-thrower BB: " << *PredBB << "\n");

                    // Find switch
                    SwitchInst *SwI = dyn_cast<SwitchInst>(PredBB->getTerminator());
                    if (!SwI) continue;
                    /* DEBUG_PRINT("Switch Instruction: " << *SwI << "\n"); */

                    // Get the condition
                    // Get condition name: switch(THISNAME)
                    Value *Cond = SwI->getCondition();
                    DEBUG_PRINT("Condition: " << *Cond << "\n");

                    LoadInst *CondLI = dyn_cast<LoadInst>(Cond);
                    if (!CondLI) continue;
                    Cond = CondLI->getPointerOperand();
                    DEBUG_PRINT("Def of Condition: " << *Cond << "\n");

                    StringRef CondName = Cond->getName();
                    if (CondName.empty()) CondName = "Condition";

                    // Find the case that matches the error
                    ConstantInt *CaseInt = SwI->findCaseDest(ErrBB);
                    if (!CaseInt) continue;

                    // Print the case
                    DEBUG_PRINT("EACCES Reason: '" << CondName << " == " << *CaseInt << "'\n\n");

                    // TODO: When to get rtlib func?
                    // Get the function to call from our runtime library.
                    LLVMContext &Ctx = ErrBB->getContext();
                    std::vector<Type*> paramTypes = {Type::getInt32Ty(Ctx)};
                    Type *retType = Type::getVoidTy(Ctx);
                    FunctionType *logFuncType = FunctionType::get(retType, paramTypes, false);
                    FunctionCallee logFunc =
                        F.getParent()->getOrInsertFunction("logcase", logFuncType);

                    // Insert a call
                    IRBuilder<> builder(ErrBB);
                    builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());
                    Value *CondStr = builder.CreateGlobalStringPtr(CondName);
                    Value* args[] = {CondStr, dyn_cast<Value>(CaseInt)};
                    builder.CreateCall(logFunc, args);
                    DEBUG_PRINT("Inserted logcase call\n");

                    // Declare the modification
                    modified = true;
                }
            }
        }
        if (modified) return PreservedAnalyses::none();
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
