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
#else
    #define DEBUG_PRINT(x) do {} while (0)
#endif // DEBUG


#ifdef DEBUG_CONCISE
    #define ISNULL(x) ((x==NULL) ? (errs() << #x << " is NULL\n", true) : (errs() << #x << " is not NULL\n", false))
#else
    #define ISNULL(x) (x==NULL)
#endif // DEBUG_CONCISE

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

    /* 
    * Backtrace from load to store
    * Get original variable (%0 is %flag)
    * code example:
      store i32 %flag, ptr %flag.addr, align 4
      %0 = load i32, ptr %flag.addr, align 4
    */
    Value* getOrigin(Value* V) {
        DEBUG_PRINT("Getting origin of: " << *V << "\n");

       // #2: %0 = load i32, ptr %flag.addr, align 4
       LoadInst *LI = dyn_cast<LoadInst>(V);
       if (!LI) return NULL;
       V = LI->getPointerOperand(); // %flag.addr

       // #1: store i32 %flag, ptr %flag.addr, align 4
       for (User *U : V->users()) {
           StoreInst *SI = dyn_cast<StoreInst>(U);
           if (!SI) continue;
           V = SI->getValueOperand(); // %flag
       }

       return V;
    }


    /*
     * Get variable name
     */
    StringRef getVarName(Value *V) {
        StringRef Name = V->getName();
        if (Name.empty()) Name = "Condition";
        return Name;
    }

    // NOTE: Expect as Function has only one ret inst
    Value* getReturnValue(Function *F) {
        for (auto &BB : *F) {
            Instruction *TI = BB.getTerminator();
            if (!TI) continue;

            // Search for return inst
            ReturnInst *RI = dyn_cast<ReturnInst>(TI);
            if (!RI) continue;

            // What is ret value?
            Value *RetVal = RI->getReturnValue();
            LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
            if (!DefLI) continue;
            RetVal = DefLI->getPointerOperand();

            return RetVal;
        }

        return NULL;
    }

    BasicBlock* getErrBB(User *U) {
        StoreInst *SI = dyn_cast<StoreInst>(U);
        if (!SI) return NULL;

        // Storing -EACCES?
        Value *ValOp = SI->getValueOperand();
        ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
        if (!CI) return NULL;
        if (CI->getSExtValue() != -EACCES) return NULL;
        DEBUG_PRINT("'return -EACCES' found!\n");

        // Error-thrower BB (BB of store -EACCES)
        BasicBlock *ErrBB = SI->getParent();
        DEBUG_PRINT("-EACCES is stored by: " << *ErrBB << "\n");

        return ErrBB;
    }

    /*
     *****************
     * main function *
     *****************
     */
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        bool modified = false;
        for (auto &F : M.functions()) {
            /* DEBUG_PRINT("Function: " << F.getName() << "\n"); */

            // TODO: Only run on may_open()
            if (F.getName() != "may_open") continue;

            Value *RetVal = getReturnValue(&F);
            if (!RetVal) continue;

        for (User *U : RetVal->users()) {
            BasicBlock *ErrBB = getErrBB(U);
            if (!ErrBB) continue;

            /*
             * if branch

              %1 = load i32, ptr %flag.addr, align 4
              %cmp = icmp eq i32 %1, 1
              br i1 %cmp, label %if.then, label %if.end

            */

            // Pred of Err-BB : BB of if
            BasicBlock *PredBB = ErrBB->getSinglePredecessor();
            if (!PredBB) continue;
            DEBUG_PRINT("Predecessor of Error-thrower BB: " << *PredBB << "\n");

            BranchInst *BrI = dyn_cast<BranchInst>(PredBB->getTerminator());
            if (!BrI) continue;
            /* DEBUG_PRINT("Branch Instruction: " << *BrI << "\n"); */

            if (BrI->getNumSuccessors() != 2) {
                DEBUG_PRINT("* Branch suc is not 2\n");
                continue;
            }


            // NOTE: Err-BB is always the first successor
            /* if (BrI->getSuccessor(1) == ErrBB) BrI->swapSuccessors(); */
            /* if (BrI->getSuccessor(0) != ErrBB) { */
            /*     DEBUG_PRINT("* Err-BB is not a successor\n"); */
            /*     continue; */
            /* } */

            /* StringRef IfCondName = getVarName(CmpI->getOperand(0)); */
            /* DEBUG_PRINT("IfCondName: " << IfCondName << "\n"); */



            // Get the condition of the if branch
            Value *IfCond = BrI->getCondition();
            if (!IfCond) continue;
            DEBUG_PRINT("Condition: " << *IfCond << "\n");

            // NOTE: IfCond is always a cmp instruction
            CmpInst *CmpI = dyn_cast<CmpInst>(IfCond);
            if (!CmpI) continue;
            DEBUG_PRINT("Cmp Instruction: " << *CmpI << "\n");

            // %tobool = icmp ne i32 %and, 0
            BinaryOperator *AndI = dyn_cast<BinaryOperator>(CmpI->getOperand(0));
            DEBUG_PRINT("AndI: " << *AndI << "\n");

            IfCond = getOrigin(AndI->getOperand(0));
            DEBUG_PRINT("if Condition: " << *IfCond << "\n");

            // bottom-up from cmp
            /* bool isEq = CmpI->isEquality(); */
            /* ConstantInt *IfCI = dyn_cast<ConstantInt>(CmpI->getOperand(1)); */

            ConstantInt *IfCI = dyn_cast<ConstantInt>(AndI->getOperand(1));

            StringRef IfCondName = getVarName(IfCond);
            DEBUG_PRINT("Reason about if: '" << IfCondName <<  " is " << *IfCI << "'\n");


            // Pred of Pred of Err-BB : BB of switch
            BasicBlock *GrandPredBB = PredBB->getSinglePredecessor();
            if (!GrandPredBB) continue;
            /* DEBUG_PRINT("Predecessor of if: " << *GrandPredBB << "\n"); */

            SwitchInst *SwI = dyn_cast<SwitchInst>(GrandPredBB->getTerminator());
            if (!SwI) continue;
            DEBUG_PRINT("Switch Instruction: " << *SwI << "\n");

            Value *SwCond = getOrigin(SwI->getCondition());
            if (!SwCond) continue;
            StringRef SwCondName = getVarName(SwCond);

            /*
             * Find case whose dest is Error-thrower BB
             */
            // Find the case that matches the error
            ConstantInt *SwCI = SwI->findCaseDest(PredBB);
            if (!SwCI) continue;

            // Print the case
            DEBUG_PRINT("Reason about switch: '" << SwCondName << " is " << *SwCI << "'\n\n");

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

            Value *CondStr = builder.CreateGlobalStringPtr(SwCondName);
            Value* args[] = {CondStr, dyn_cast<Value>(SwCI)};
            builder.CreateCall(logFunc, args);

            CondStr = builder.CreateGlobalStringPtr(IfCondName);
            args[0] = CondStr;
            args[1] = dyn_cast<Value>(IfCI);
            builder.CreateCall(logFunc, args);

            // Declare the modification
            modified = true;
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
