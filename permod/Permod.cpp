#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

/* #define DEBUG */
#ifdef DEBUG
    #define DEBUG_PRINT(x) do { errs() << x; } while (0)
#else
    #define DEBUG_PRINT(x) do {} while (0)
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


            // Search for return BB (Contains return instruction)
            for (auto &BB : F) {
                Instruction *TI = BB.getTerminator();
                if (!TI) continue;

                // Check if this BB has a return instruction
                ReturnInst *RI = dyn_cast<ReturnInst>(TI);
                if (!RI) continue;
                DEBUG_PRINT("~Found BB of Return\n");

                // Find when retval is loaded (load i32, ptr %retval)
                Value *RetVal = RI->getReturnValue();
                LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
                if (!DefLI) continue;
                RetVal = DefLI->getPointerOperand();

                // Find when retval is stored (store i32 -13, ptr %retval)
                for (User *U : RetVal->users()) {
                    DEBUG_PRINT("Checking User: " << *U << "\n");

                    StoreInst *SI = dyn_cast<StoreInst>(U);
                    if (!SI) continue;

                    // Check if storing value is -EACCES
                    Value *ValOp = SI->getValueOperand();
                    ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
                    if (!CI) continue;
                    if (CI->getSExtValue() != -EACCES) continue;
                    DEBUG_PRINT("'return -EACCES' found!\n");

                    // Error-thrower BB (BB of store -EACCES)
                    BasicBlock *ErrBB = SI->getParent();
                    DEBUG_PRINT("-EACCES is stored by: " << *ErrBB << "\n");

                    // Pred of Err-BB : BB of if
                    BasicBlock *PredBB = ErrBB->getSinglePredecessor();
                    if (!PredBB) continue;
                    /* DEBUG_PRINT("Predecessor of Error-thrower BB: " << *PredBB << "\n"); */

                    BranchInst *BrI = dyn_cast<BranchInst>(PredBB->getTerminator());
                    if (!BrI) continue;
                    /* DEBUG_PRINT("Branch Instruction: " << *BrI << "\n"); */

                    if (BrI->getNumSuccessors() != 2) {
                        DEBUG_PRINT("* Branch suc is not 2\n");
                        continue;
                    }

                    // NOTE: Err-BB is always the first (true) successor
                    if (BrI->getSuccessor(0) != ErrBB) {
                        DEBUG_PRINT("* Err-BB is not the first successor\n");
                        continue;
                    }


                    // Pred of Pred of Err-BB : BB of switch
                    BasicBlock *GrandPredBB = PredBB->getSinglePredecessor();
                    if (!GrandPredBB) continue;
                    DEBUG_PRINT("Predecessor of if: " << *GrandPredBB << "\n");

                    SwitchInst *SwI = dyn_cast<SwitchInst>(GrandPredBB->getTerminator());
                    if (!SwI) continue;
                    DEBUG_PRINT("Switch Instruction: " << *SwI << "\n");

                    /* 
                     * Get condition name: switch(THISNAME)
                     * code example:
                       store i32 %flag, ptr %flag.addr, align 4
                       %0 = load i32, ptr %flag.addr, align 4
                       switch i32 %0, label %sw.default [
                    */
                    // #3: use of condition 
                    Value *Cond = SwI->getCondition();
                    DEBUG_PRINT("Condition: " << *Cond << "\n");

                    // #2: def of condition (ptr)
                    LoadInst *CondLI = dyn_cast<LoadInst>(Cond);
                    if (!CondLI) continue;
                    Cond = CondLI->getPointerOperand();
                    DEBUG_PRINT("Def of Condition: " << *Cond << "\n");

                    // #1: use-def of condition (pointed original value)
                    for (User *UC : Cond->users()) { // User of Condition
                        DEBUG_PRINT("Checking User: " << *UC << "\n");
                        StoreInst *CondSI = dyn_cast<StoreInst>(UC);
                        if (!CondSI) continue;
                        DEBUG_PRINT("Store Instruction: " << *CondSI << "\n");
                        Cond = CondSI->getValueOperand();
                    }

                    // Get the name (default to "Condition")
                    StringRef CondName = Cond->getName();
                    if (CondName.empty()) CondName = "Condition";

                    /*
                     * Find case whose dest is Error-thrower BB
                     */
                    // Find the case that matches the error
                    ConstantInt *CaseInt = SwI->findCaseDest(PredBB);
                    if (!CaseInt) continue;

                    // Print the case
                    DEBUG_PRINT("EACCES Reason: '" << CondName << " == " << *CaseInt << "'\n\n");

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
