#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <errno.h>

#define MAX_TRACE_DEPTH 3

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
            Name = "Unnamed Condition";
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

        // Error-thrower BB (BB of store -EACCES)
        BasicBlock *ErrBB = SI->getParent();

        return ErrBB;
    }

    class Condition {
      public:
        StringRef Name;
        Value *Val;
        Condition(StringRef Name, Value *Val) : Name(Name), Val(Val) {}
    };

    /*
     * Get name & value of if condition

     * Returns: (StringRef, Value*)
       - if (flag & val) {}     // name:of flag, val:val
       - if (!func()) {}   // name:of func, val:0

     * Kinds of if condition:
       - AndInst: if (flag & 2) {}
            %1 = load i32, ptr %flag.addr, align 4
            %and = and i32 %1, 2
            %tobool = icmp ne i32 %and, 0
       - CallInst: if (!func()) {}
            %call = call i32 @function()
            %tobool = icmp ne i32 %call, 0
    */
    // TODO: Predicts only true branch
    Condition *getIfCond(BranchInst *BrI) {
        DEBUG_PRINT("Getting if condition from: " << *BrI << "\n");

        StringRef name;
        Value *val;

        // Get if condition
        // NOTE: IfCond is always a cmp instruction
        /*
           Kinds of CmpInst:
               %tobool = icmp ne i32 %and, 0
               %tobool = icmp ne i32 %call, 0
         */
        Value *IfCond = BrI->getCondition();
        if (!IfCond)
            return NULL;
        CmpInst *CmpI = dyn_cast<CmpInst>(IfCond);
        if (!CmpI)
            return NULL;
        DEBUG_PRINT("Cmp Instruction: " << *CmpI << "\n");

        // Get name & val
        if (auto *AndI = dyn_cast<BinaryOperator>(CmpI->getOperand(0))) {
            DEBUG_PRINT("AndInst: " << *AndI << "\n");
            IfCond = getOrigin(AndI->getOperand(0));
            if (!IfCond)
                return NULL;
            name = getVarName(IfCond);
            val = AndI->getOperand(1);
        } else if (auto *CallI = dyn_cast<CallInst>(CmpI->getOperand(0))) {
            DEBUG_PRINT("CallInst: " << *CallI << "\n");
            Function *Callee = CallI->getCalledFunction();
            if (!Callee)
                return NULL;
            name = getVarName(Callee);
            val = CmpI->getOperand(1);
        } else {
            DEBUG_PRINT("Unexpected Instruction: " << *CmpI->getOperand(0)
                                                   << "\n");
            return NULL;
        }

        return new Condition(name, val);
    }

    /* Get name & value of switch condition
     * Returns: (StringRef, Value*)
       - switch (flag) {}     // name:of flag, val:of flag
     */
    Condition *dyn_getSwCond(SwitchInst *SwI) {
        StringRef name;
        Value *val;

        Value *SwCond = SwI->getCondition();
        DEBUG_PRINT("Switch condition: " << *SwCond << "\n");

        // Get value
        val = SwCond;
        DEBUG_PRINT("Switch value: " << *val << "\n");

        // Get name
        SwCond = getOrigin(SwCond);
        if (!SwCond)
            return NULL;
        DEBUG_PRINT("Original Switch condition: " << *SwCond << "\n");

        name = getVarName(SwCond);
        DEBUG_PRINT("Switch name: " << name << "\n");

        return new Condition(name, val);
    }

    /*
     * Backtrace to find Switch Statement BB
     * Returns: BasicBlock*
     */
    BasicBlock *getSwBB(BasicBlock *BB) {
        for (int i = 0; i < MAX_TRACE_DEPTH; i++) {
            for (auto *PredBB : predecessors(BB)) {
                if (isa<SwitchInst>(PredBB->getTerminator())) {
                    DEBUG_PRINT("Switch BB: " << *PredBB << "\n");
                    return PredBB;
                }
                BB = PredBB;
            }
        }
        return NULL;
    }


    /*
     *****************
     * main function *
     *****************
     */
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        bool modified = false;
        for (auto &F : M.functions()) {

            Value *RetVal = getReturnValue(&F);
            if (!RetVal)
                continue;

            for (User *U : RetVal->users()) {

                // Get Error-thrower BB "ErrBB"
                /* store -13, ptr %1, align 4 */
                BasicBlock *ErrBB = getErrBB(U);
                if (!ErrBB)
                    continue;

                DEBUG_PRINT("\n///////////////////////////////////////\n");
                DEBUG_PRINT(F.getName() << " has 'return -EACCES'\n");
                DEBUG_PRINT("Error-thrower BB: " << *ErrBB << "\n");

                // Get If statement BB "IfBB"; Pred of ErrBB
                BasicBlock *IfBB = ErrBB->getSinglePredecessor();
                if (!IfBB) {
                    DEBUG_PRINT("* ErrBB has no single predecessor\n");
                    continue;
                }
                DEBUG_PRINT("If statement BB (Pred of ErrBB): " << *IfBB
                                                                << "\n");

                // Get If condition
                // TODO: "if(A && B){}" is not supported.
                // This splits into two BBs; "if(A){ if(B){} }"
                /*
                 * if branch
                  %1 = load i32, ptr %flag.addr, align 4
                  %cmp = icmp eq i32 %1, 1
                  br i1 %cmp, label %if.then, label %if.end
                */
                BranchInst *BrI = dyn_cast<BranchInst>(IfBB->getTerminator());
                if (!BrI) {
                    DEBUG_PRINT("* IfBB terminator is not a branch\n");
                    continue;
                }
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
                   switch i32 %1, label %sw.default [
                     // This is one of the cases
                     i32 0, label %sw.bb
                     // Sometimes 1 case has multiple preds
                     i32 1, label %sw.bb1
                     i32 2, label %sw.bb1
                   ]
                 */
                BasicBlock *SwBB = getSwBB(IfBB);
                if (!SwBB) {
                    DEBUG_PRINT("* Switch BB is NULL\n");
                    delete IfCond;
                    continue;
                }
                DEBUG_PRINT("Switch BB: " << *SwBB << "\n");

                // NOTE: getSwBB() already knows that SwI is not NULL
                SwitchInst *SwI = dyn_cast<SwitchInst>(SwBB->getTerminator());
                if (!SwI) {
                    DEBUG_PRINT("* Switch Instruction is NULL\n");
                    delete IfCond;
                    continue;
                }
                DEBUG_PRINT("Switch Instruction: " << *SwI << "\n");

                Condition *SwCond = dyn_getSwCond(SwI);
                if (!SwCond) {
                    DEBUG_PRINT("* SwCond is NULL\n");
                    delete IfCond;
                    continue;
                }


                // Prepare function
                LLVMContext &Ctx = ErrBB->getContext();
                std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
                Type *retType = Type::getVoidTy(Ctx);
                FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
                FunctionCallee logFunc = F.getParent()->getOrInsertFunction("_printk", funcType);

                // Insert a call
                IRBuilder<> builder(ErrBB);
                builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());

                std::vector<Value *> args;
                Twine format = Twine("[PERMOD] %s: %d\n");
                Value *formatStr = builder.CreateGlobalStringPtr(format.getSingleStringRef());

                args.push_back(builder.CreatePointerCast(formatStr, Type::getInt8PtrTy(Ctx)));
                args.push_back(builder.CreateGlobalStringPtr(SwCond->Name));
                args.push_back(SwCond->Val);
                builder.CreateCall(logFunc, args);
                DEBUG_PRINT("Inserted log for switch\n");
                args.clear();

                args.push_back(builder.CreatePointerCast(formatStr, Type::getInt8PtrTy(Ctx)));
                args.push_back(builder.CreateGlobalStringPtr(IfCond->Name));
                args.push_back(IfCond->Val);
                builder.CreateCall(logFunc, args);
                DEBUG_PRINT("Inserted log for if\n");

                // release memory
                delete IfCond;
                delete SwCond;

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
