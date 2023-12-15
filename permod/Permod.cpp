#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"

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
        StringRef Name = getOrigin(V)->getName();
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

        return nullptr;
    }

    /* Get error-thrower BB "ErrBB"
       store i32 -13，ptr %1, align 4
     * Returns: BasicBlock*
     */
    BasicBlock *getErrBB(User *U) {
        StoreInst *SI = dyn_cast<StoreInst>(U);
        if (!SI)
            return nullptr;

        // Storing -EACCES?
        Value *ValOp = SI->getValueOperand();
        ConstantInt *CI = dyn_cast<ConstantInt>(ValOp);
        if (!CI)
            return nullptr;
        if (CI->getSExtValue() != -EACCES)
            return nullptr;

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


    // Check whether the DestBB is true or false successor
    bool isBranchTrue(BranchInst *BrI, BasicBlock *DestBB) {
        if (BrI->getSuccessor(0) == DestBB)
            return true;
        return false;
    }

    /* Get name & value of if condition
       - C
        if (flag & 2) {}     // name:of flag, val:val
       - IR
        %and = and i32 %flag, 2
        %tobool = icmp ne i32 %and, 0
        br i1 %tobool, label %if.then, label %if.end
     */
    Condition* getIfCond_cmp(BranchInst *BrI, CmpInst *CmpI, BasicBlock *DestBB) {
        StringRef name;
        Value *val;

        Value *CmpOp = CmpI->getOperand(0);
        if (!CmpOp)
            return nullptr;

        // CmpOp: %and = and i32 %flag, 2
        if (auto *AndI = dyn_cast<BinaryOperator>(CmpOp)) {
            DEBUG_PRINT("AndInst: " << *AndI << "\n");
            CmpOp = AndI->getOperand(0);
            if (!CmpOp)
                return nullptr;
            DEBUG_PRINT("AndInst operand: " << *CmpOp << "\n");
            name = getVarName(CmpOp);
            val = AndI->getOperand(1);
            return new Condition(name, val);
        }

        // CmpOp: %call = call i32 @function()
        if (auto *CallI = dyn_cast<CallInst>(CmpOp)) {
            DEBUG_PRINT("CallInst: " << *CallI << "\n");
            Function *Callee = CallI->getCalledFunction();
            if (!Callee)
                return nullptr;
            name = getVarName(Callee);
            int valInt = isBranchTrue(BrI, DestBB) ? 1 : 0;
            val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), valInt);
            return new Condition(name, val);
        }

        DEBUG_PRINT("Unexpected Instruction: " << *CmpOp << "\n");
        return nullptr;
    }

    /* Get name & value of if condition
       - C
        if (!func()) {}   // name:of func, val:0
       - IR
        %call = call i32 @function()
        br i1 %call, label %if.then, label %if.end
     */
    Condition* getIfCond_call(BranchInst *BrI, CallInst *CallI, BasicBlock *DestBB) {
        StringRef name;
        Value *val;

        Function *Callee = CallI->getCalledFunction();
        if (!Callee)
            return nullptr;

        name = getVarName(Callee);
        int valInt = isBranchTrue(BrI, DestBB) ? 1 : 0;
        val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), valInt);
        return new Condition(name, val);
    }

    /* Get name & value of if condition
       - And condition
        if (flag & 2) {}
       - Call condition
        if (!func()) {}   // name:of func, val:0
     */
    Condition* getIfCond(BranchInst *BrI, BasicBlock *DestBB, std::vector<Condition *> &conds) {
        StringRef name;
        Value *val;

        Value *IfCond = BrI->getCondition();
        if (!IfCond)
            return nullptr;

        // And condition: if (flag & 2) {}
        if (isa<CmpInst>(IfCond)) {
            DEBUG_PRINT("IfCond is Cmp: " << *IfCond << "\n");
            return getIfCond_cmp(BrI, dyn_cast<CmpInst>(IfCond), DestBB);
        }
        // Call condition: if (!func()) {}
        if (isa<CallInst>(IfCond)) {
            DEBUG_PRINT("IfCond is Call: " << *IfCond << "\n");
            return getIfCond_call(BrI, dyn_cast<CallInst>(IfCond), DestBB);
        }
        DEBUG_PRINT("Unexpected Instruction: " << *IfCond << "\n");
        return nullptr;
    }

    /* Get name & value of switch condition
     * Returns: (StringRef, Value*)
       - switch (flag) {}     // name:of flag, val:of flag
     */
    Condition* getSwCond(SwitchInst *SwI) {
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
            return nullptr;
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
        return nullptr;
    }

    void deleteAllCond(std::vector<Condition *> &conds) {
        while (!conds.empty()) {
            delete conds.back();
            conds.pop_back();
        }
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
                /* DEBUG_PRINT(*(dyn_cast<StoreInst>(U)->getValueOperand()) <<
                 * "!!\n"); */

                // Prepare Array of Condition
                std::vector<Condition *> conds;

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
                Condition *cond = getIfCond(BrI, ErrBB, conds);
                if (!cond) {
                    DEBUG_PRINT("* getIfCond has failed.\n");
                    deleteAllCond(conds);
                    continue;
                }
                conds.push_back(cond);

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
                    deleteAllCond(conds);
                    continue;
                }
                DEBUG_PRINT("Switch BB: " << *SwBB << "\n");

                // NOTE: getSwBB() already knows that SwI is not NULL
                SwitchInst *SwI = dyn_cast<SwitchInst>(SwBB->getTerminator());
                if (!SwI) {
                    DEBUG_PRINT("* Switch Instruction is NULL\n");
                    deleteAllCond(conds);
                    continue;
                }
                DEBUG_PRINT("Switch Instruction: " << *SwI << "\n");

                cond = getSwCond(SwI);
                if (!cond) {
                    DEBUG_PRINT("* geSwCond has failed.\n");
                    deleteAllCond(conds);
                    continue;
                }
                conds.push_back(cond);

                /* Insert log */
                // Prepare builder
                IRBuilder<> builder(ErrBB);
                builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());
                LLVMContext &Ctx = ErrBB->getContext();

                /*
                // Debug info
                // NOTE: need clang flag "-g"
                StringRef filename = F.getParent()->getSourceFileName();
                const DebugLoc &Loc = dyn_cast<Instruction>(U)->getDebugLoc();
                unsigned line = Loc->getLine();
                DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
                Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
                conds.push_back(new Condition(F.getName(),
                dyn_cast<StoreInst>(U)->getValueOperand())); conds.push_back(new
                Condition(filename, lineVal));
                */

                // Prepare function
                std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
                Type *retType = Type::getVoidTy(Ctx);
                FunctionType *funcType =
                    FunctionType::get(retType, paramTypes, false);
                FunctionCallee logFunc =
                    F.getParent()->getOrInsertFunction("printf", funcType);

                // Prepare arguments
                std::vector<Value *> args;
                Twine format = Twine("[PERMOD] %s: %d\n");
                Value *formatStr =
                    builder.CreateGlobalStringPtr(format.getSingleStringRef());

                while (!conds.empty()) {
                    Condition *cond = conds.back();
                    conds.pop_back();

                    args.push_back(builder.CreatePointerCast(
                        formatStr, Type::getInt8PtrTy(Ctx)));
                    args.push_back(builder.CreateGlobalStringPtr(cond->Name));
                    args.push_back(cond->Val);
                    builder.CreateCall(logFunc, args);
                    DEBUG_PRINT("Inserted log for " << cond->Name << "\n");
                    args.clear();

                    delete cond;
                }

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
