#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)                                                         \
    do {                                                                       \
        errs() << x;                                                           \
    } while (0)
#else
#define DEBUG_PRINT(x)                                                         \
    do {                                                                       \
    } while (0)
#endif // DEBUG

#ifdef DEBUG_CONCISE
#define ISNULL(x)                                                              \
    ((x == NULL) ? (errs() << #x << " is NULL\n", true)                        \
                 : (errs() << #x << " is not NULL\n", false))
#else
#define ISNULL(x) (x == NULL)
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
    Value *getOrigin(Value *V) {
        DEBUG_PRINT("Getting origin of: " << *V << "\n");

        if (auto *LI = dyn_cast<LoadInst>(V)) {
            V = LI->getPointerOperand();
            DEBUG_PRINT("LoadInst: " << *V << "\n");
        } else if (auto *AI = dyn_cast<BinaryOperator>(V)) {
            V = AI->getOperand(0);
            DEBUG_PRINT("BinaryOperator: " << *V << "\n");
        } else {
            DEBUG_PRINT("Unknown: " << *V << "\n");
            return V;
        }

        // #1: store i32 %flag, ptr %flag.addr, align 4
        for (User *U : V->users()) {
            StoreInst *SI = dyn_cast<StoreInst>(U);
            if (!SI)
                continue;
            V = SI->getValueOperand(); // %flag
        }

        // TODO: Special case for kernel
        if (auto *ZI = dyn_cast<ZExtInst>(V)) {
            V = ZI->getOperand(0);
            DEBUG_PRINT("ZExtInst: " << *V << "\n");
            if (auto *ZLI = dyn_cast<LoadInst>(V)) {
                V = ZLI->getOperand(0);
                DEBUG_PRINT("LoadInst: " << *V << "\n");
            }
        }

        return V;
    }

    /*
     * Get variable name
     */
    StringRef getVarName(Value *V) {
        StringRef Name = V->getName();
        if (Name.empty())
            Name = "Condition";
        return Name;
    }

    // NOTE: Expect as Function has only one ret inst
    Value *getReturnValue(Function *F) {
        for (auto &BB : *F) {
            Instruction *TI = BB.getTerminator();
            if (!TI)
                continue;

            // Search for return inst
            ReturnInst *RI = dyn_cast<ReturnInst>(TI);
            if (!RI)
                continue;

            // What is ret value?
            Value *RetVal = RI->getReturnValue();
            if (!RetVal)
                continue;
            LoadInst *DefLI = dyn_cast<LoadInst>(RetVal);
            if (!DefLI)
                continue;
            RetVal = DefLI->getPointerOperand();

            return RetVal;
        }

        return NULL;
    }

    /* Get error-thrower BB "ErrBB"
       store i32 -13，ptr %1, align 4
     * Returns: BasicBlock*
     */
    BasicBlock *getErrBB(User *U) {
        StoreInst *SI = dyn_cast<StoreInst>(U);
        if (!SI)
            return NULL;

        // Storing -EACCES?
        Value *ValOp = SI->getValueOperand();
        ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
        if (!CI)
            return NULL;
        if (CI->getSExtValue() != -EACCES)
            return NULL;
        DEBUG_PRINT("'return -EACCES' found!\n");

        // Error-thrower BB (BB of store -EACCES)
        BasicBlock *ErrBB = SI->getParent();
        DEBUG_PRINT("-EACCES is stored by: " << *ErrBB << "\n");

        return ErrBB;
    }

    class Condition {
      public:
        StringRef Name;
        ConstantInt *Val;
        Condition(StringRef Name, ConstantInt *Val) : Name(Name), Val(Val) {}
    };

    // Get if condition
    // Returns: (Value*, ConstantInt)
    // TODO: Predicts only true branch
    Condition *getIfCond(BranchInst *BrI) {
        // Get the condition of the if branch
        Value *IfCond = BrI->getCondition();
        if (!IfCond)
            return NULL;
        DEBUG_PRINT("Condition: " << *IfCond << "\n");

        // NOTE: IfCond is always a cmp instruction
        CmpInst *CmpI = dyn_cast<CmpInst>(IfCond);
        if (!CmpI)
            return NULL;
        DEBUG_PRINT("Cmp Instruction: " << *CmpI << "\n");

        // %tobool = icmp ne i32 %and, 0
        BinaryOperator *AndI = dyn_cast<BinaryOperator>(CmpI->getOperand(0));
        if (!AndI)
            return NULL;
        DEBUG_PRINT("AndI: " << *AndI << "\n");

        IfCond = getOrigin(AndI->getOperand(0));
        if (!IfCond)
            return NULL;
        DEBUG_PRINT("if Condition: " << *IfCond << "\n");

        // bottom-up from cmp
        /* bool isEq = CmpI->isEquality(); */
        /* ConstantInt *IfCI = dyn_cast<ConstantInt>(CmpI->getOperand(1)); */

        ConstantInt *IfCI = dyn_cast<ConstantInt>(AndI->getOperand(1));

        StringRef IfCondName = getVarName(IfCond);
        DEBUG_PRINT("Reason about if: '" << IfCondName << " is " << *IfCI
                                         << "'\n");

        return new Condition(IfCondName, IfCI);
    }

    class SwCondition {
      public:
        StringRef Name;
        Value *Val;
        SwCondition(StringRef Name, Value *Val) : Name(Name), Val(Val) {}
    };

    SwCondition *dyn_getSwCond(SwitchInst *SwI) {
        Value *SwCond = getOrigin(SwI->getCondition());
        if (!SwCond)
            return NULL;
        StringRef SwCondName = getVarName(SwCond);
        DEBUG_PRINT("Switch name: " << SwCondName << "\n");

        return new SwCondition(SwCondName, SwCond);
    }

    // Prepare function
    FunctionCallee prepareFunction(std::string funcName, Function &F,
                                   LLVMContext &Ctx) {
        std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
        Type *retType = Type::getVoidTy(Ctx);
        FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
        FunctionCallee newFunc =
            F.getParent()->getOrInsertFunction(funcName, funcType);

        return newFunc;
    }

    /*
     *****************
     * main function *
     *****************
     */
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        bool modified = false;
        for (auto &F : M.functions()) {
            DEBUG_PRINT("Function: " << F.getName() << "\n");

            // TODO: Only run on may_open()
            /* if (F.getName() != "may_open") continue; */
            /* DEBUG_PRINT("may_open found!\n"); */

            Value *RetVal = getReturnValue(&F);
            if (!RetVal)
                continue;

            for (User *U : RetVal->users()) {

                // Get Error-thrower BB "ErrBB"
                /* store -13, ptr %1, align 4 */
                BasicBlock *ErrBB = getErrBB(U);
                if (!ErrBB)
                    continue;

                // Get If statement BB "IfBB"; Pred of ErrBB
                BasicBlock *IfBB = ErrBB->getSinglePredecessor();
                if (!IfBB)
                    continue;
                DEBUG_PRINT("If statement BB (Pred of ErrBB): " << *IfBB
                                                                << "\n");


                // Get If condition
                /*
                 * if branch
                  %1 = load i32, ptr %flag.addr, align 4
                  %cmp = icmp eq i32 %1, 1
                  br i1 %cmp, label %if.then, label %if.end
                */
                BranchInst *BrI = dyn_cast<BranchInst>(IfBB->getTerminator());
                if (!BrI)
                    continue;
                /* DEBUG_PRINT("Branch Instruction: " << *BrI << "\n"); */

                if (BrI->getNumSuccessors() != 2) {
                    DEBUG_PRINT("* Branch suc is not 2\n");
                    continue;
                }

                // NOTE: Err-BB is always the first successor
                /* if (BrI->getSuccessor(1) == ErrBB) BrI->swapSuccessors();
                 */
                /* if (BrI->getSuccessor(0) != ErrBB) { */
                /*     DEBUG_PRINT("* Err-BB is not a successor\n"); */
                /*     continue; */
                /* } */

                Condition *IfCond = getIfCond(BrI);
                if (!IfCond) {
                    DEBUG_PRINT("* IfCond is NULL\n");
                    continue;
                }


                // Get Switch condition
                /*
                 * switch i32 %1, label %sw.default [
                 *   // This is one of the cases
                 *   i32 0, label %sw.bb
                 *   // Sometimes 1 case has multiple preds
                 *   i32 1, label %sw.bb1
                 *   i32 2, label %sw.bb1
                 * ]
                 */

                // NOTE: CaseBB (Case of switch) is IfBB
                BasicBlock *CaseBB = IfBB;
                SwitchInst *SwI;

                // Get multiple preds of CaseBB, which is case of switch
                for (auto *BB : predecessors(CaseBB)) {
                    DEBUG_PRINT("Getting preds:" << *BB << "\n");
                    SwI = dyn_cast<SwitchInst>(BB->getTerminator());
                    if (SwI)
                        break;
                }
                if (!SwI) {
                    DEBUG_PRINT("* SwitchInst is NULL\n");
                    continue;
                }
                DEBUG_PRINT("Switch Instruction: " << *SwI << "\n");

                SwCondition *SwCond = dyn_getSwCond(SwI);

                // TODO:
                /*
                switch()
                case:
                    if() return -EISDIR;
                    if() return -EACCES; //HERE!!
                */


                // Prepare function
                FunctionCallee logFunc =
                    prepareFunction("logcase", F, ErrBB->getContext());

                // Insert a call
                IRBuilder<> builder(ErrBB);
                builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());

                Value *CondStr;
                Value *args[2];

                CondStr = builder.CreateGlobalStringPtr(SwCond->Name);
                args[0] = CondStr;
                args[1] = SwCond->Val;
                builder.CreateCall(logFunc, args);
                DEBUG_PRINT("Inserted log for switch\n");

                CondStr = builder.CreateGlobalStringPtr(IfCond->Name);
                args[0] = CondStr;
                args[1] = dyn_cast<Value>(IfCond->Val);
                builder.CreateCall(logFunc, args);
                DEBUG_PRINT("Inserted log for if\n");

                // Declare the modification
                modified = true;
            }
        }
        if (modified)
            return PreservedAnalyses::none();
        return PreservedAnalyses::all();
    };
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {.APIVersion = LLVM_PLUGIN_API_VERSION,
            .PluginName = "Permod pass",
            .PluginVersion = "v0.1",
            .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
                PB.registerPipelineStartEPCallback(
                    [](ModulePassManager &MPM, OptimizationLevel Level) {
                        MPM.addPass(PermodPass());
                    });
            }};
}
