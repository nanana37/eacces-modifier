#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"

#include <errno.h>

#define MAX_TRACE_DEPTH 100

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
        }

        if (auto *AI = dyn_cast<BinaryOperator>(V)) {
            V = AI->getOperand(0);
        }

        // #1: store i32 %flag, ptr %flag.addr, align 4
        for (User *U : V->users()) {
            StoreInst *SI = dyn_cast<StoreInst>(U);
            if (!SI)
                continue;
            V = SI->getValueOperand(); // %flag
        }

        // NOTE: Special case for kernel
        if (auto *ZI = dyn_cast<ZExtInst>(V)) {
            V = ZI->getOperand(0);
            if (auto *ZLI = dyn_cast<LoadInst>(V)) {
                V = ZLI->getOperand(0);
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

    /*
     * Condition
     */
    // Condition type: This is for type of log
    // NOTE: Used for array index!!
    enum CondType {
        CMPTRUE = 0,
        CMPFALSE,
        CALLTRUE,
        CALLFALSE,
        SWITCH,
        DINFO,
        HELLO,
        NUM_OF_CONDTYPE
    };

    class Condition {
      public:
        StringRef Name;
        Value *Val;
        CondType Type;

        Condition(StringRef name, Value *val, CondType type)
            : Name(name), Val(val), Type(type) {}
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
    Condition *getIfCond_cmp(BranchInst *BrI, CmpInst *CmpI,
                             BasicBlock *DestBB) {
        StringRef name;
        Value *val;
        CondType type;

        Value *CmpOp = CmpI->getOperand(0);
        if (!CmpOp)
            return nullptr;

        // CmpOp: %and = and i32 %flag, 2
        if (auto *AndI = dyn_cast<BinaryOperator>(CmpOp)) {
            DEBUG_PRINT("AndInst: " << *AndI << "\n");
            CmpOp = AndI->getOperand(0);
            if (!CmpOp)
                return nullptr;
            name = getVarName(CmpOp);
            val = AndI->getOperand(1);
            type = isBranchTrue(BrI, DestBB) ? CMPTRUE : CMPFALSE;
            return new Condition(name, val, type);
        }

        // CmpOp: %call = call i32 @function()
        if (auto *CallI = dyn_cast<CallInst>(CmpOp)) {
            DEBUG_PRINT("CallInst: " << *CallI << "\n");
            Function *Callee = CallI->getCalledFunction();
            if (!Callee)
                return nullptr;
            name = getVarName(Callee);
            int valInt = isBranchTrue(BrI, DestBB) ? 1 : 0;
            val =
                ConstantInt::get(Type::getInt32Ty(CallI->getContext()), valInt);
            type = isBranchTrue(BrI, DestBB) ? CALLTRUE : CALLFALSE;
            return new Condition(name, val, type);
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
    Condition *getIfCond_call(BranchInst *BrI, CallInst *CallI,
                              BasicBlock *DestBB) {
        StringRef name;
        Value *val;
        CondType type;

        Function *Callee = CallI->getCalledFunction();
        if (!Callee)
            return nullptr;

        name = getVarName(Callee);
        int valInt = isBranchTrue(BrI, DestBB) ? 1 : 0;
        val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), valInt);
        type = isBranchTrue(BrI, DestBB) ? CALLTRUE : CALLFALSE;
        return new Condition(name, val, type);
    }

    /* Get name & value of if condition
       - And condition
        if (flag & 2) {}
       - Call condition
        if (!func()) {}   // name:of func, val:0
     */
    Condition *getIfCond(BranchInst *BrI, BasicBlock *DestBB) {
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
    Condition *getSwCond(SwitchInst *SwI) {
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

        return new Condition(name, val, SWITCH);
    }

    /*
     * Search predecessors for If/Switch Statement BB (CondBB)
     * Returns: BasicBlock*
     */
    BasicBlock *searchPredsForCondBB(BasicBlock *BB) {
        DEBUG_PRINT("Search preds of: " << *BB << "\n");
        for (auto *PredBB : predecessors(BB)) {
            Instruction *TI = PredBB->getTerminator();
            if (isa<BranchInst>(TI)) {
                DEBUG_PRINT("Found BranchInst: " << *TI << "\n");
                // Branch sometimes has only one successor
                // e.g. br label %if.end
                if (TI->getNumSuccessors() != 2)
                    continue;
                return PredBB;
            }
            if (isa<SwitchInst>(TI)) {
                DEBUG_PRINT("Found SwitchInst: " << *TI << "\n");
                return PredBB;
            }
            BB = PredBB;
        }
        return nullptr;
    }

    /*
     * Get Condition from CondBB
     */
    Condition *getCondFromCondBB(BasicBlock *CondBB, BasicBlock *DestBB) {
        DEBUG_PRINT("Getting condition\n");

        // Get Condition
        /*
         * if (flag & 2) {}     // name:of flag, val:val
         * switch (flag) {}     // name:of flag, val:of flag
         */
        if (auto *BrI = dyn_cast<BranchInst>(CondBB->getTerminator())) {
            return getIfCond(BrI, DestBB);
        } else if (auto *SwI = dyn_cast<SwitchInst>(CondBB->getTerminator())) {
            return getSwCond(SwI);
        } else {
            DEBUG_PRINT("* CondBB terminator is not a branch or switch\n");
            return nullptr;
        }
    }

    void backTraceToGetCondOfErrBB(BasicBlock *ErrBB,
                                   std::vector<Condition *> &conds) {
        BasicBlock *BB = ErrBB;
        for (int i = 0; i < MAX_TRACE_DEPTH; i++) {

            // Get Condition BB
            BasicBlock *CondBB = searchPredsForCondBB(BB);
            if (!CondBB) {
                DEBUG_PRINT("* No CondBB in preds\n");
                break;
            }

            // Get Condition
            Condition *cond = getCondFromCondBB(CondBB, BB);
            if (!cond) {
                DEBUG_PRINT("* getCondFromCondBB has failed.\n");
                break;
            }

            // Update BB
            // NOTE: We need CondBB, not only its terminator
            BB = CondBB;

            // Add Condition
            conds.push_back(cond);

            // The loop reaches to the end?
            if (CondBB == &CondBB->getParent()->getEntryBlock()) {
                DEBUG_PRINT("* Reached to the entry\n");
                break;
            }
            if (i == MAX_TRACE_DEPTH - 1) {
                DEBUG_PRINT("* Reached to the max depth\n");
                break;
            }
        }
    }

    /*
     * Delete all conditions
     * Call this when you continue to next ErrBB
     */
    void deleteAllCond(std::vector<Condition *> &conds) {
        while (!conds.empty()) {
            delete conds.back();
            conds.pop_back();
        }
    }

    /*
     * Prepare format string
     */
    void prepareFormat(Value *format[], IRBuilder<> &builder,
                       LLVMContext &Ctx) {
        StringRef formatStr[NUM_OF_CONDTYPE];
        formatStr[CMPTRUE] = "[Permod] %s: %d (true)\n";
        formatStr[CMPFALSE] = "[Permod] %s: %d (false)\n";
        formatStr[CALLTRUE] = "[Permod] %s(): %d (true)\n";
        formatStr[CALLFALSE] = "[Permod] %s(): %d (false)\n";
        formatStr[SWITCH] = "[Permod] %s: %d (switch)\n";
        formatStr[DINFO] = "[Permod] %s: %d\n";
        formatStr[HELLO] = "--- Hello, I'm Permod ---\n";

        for (int i = 0; i < NUM_OF_CONDTYPE; i++) {
            Value *formatVal = builder.CreateGlobalStringPtr(formatStr[i]);
            format[i] =
                builder.CreatePointerCast(formatVal, Type::getInt8PtrTy(Ctx));
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

                // Backtrace to find If/Switch Statement BB
                backTraceToGetCondOfErrBB(ErrBB, conds);
                if (conds.empty()) {
                    DEBUG_PRINT("** conds is empty\n");
                    continue;
                }

                /* Insert log */
                DEBUG_PRINT("...Inserting log...\n");
                // Prepare builder
                IRBuilder<> builder(ErrBB);
                builder.SetInsertPoint(ErrBB, ErrBB->getFirstInsertionPt());
                LLVMContext &Ctx = ErrBB->getContext();

                // Debug info
                // NOTE: need clang flag "-g"
                StringRef filename = F.getParent()->getSourceFileName();
                const DebugLoc &Loc = dyn_cast<Instruction>(U)->getDebugLoc();
                unsigned line = Loc->getLine();
                DEBUG_PRINT("Debug info: " << filename << ":" << line << "\n");
                Value *lineVal = ConstantInt::get(Type::getInt32Ty(Ctx), line);
                conds.push_back(new Condition(
                    F.getName(), dyn_cast<StoreInst>(U)->getValueOperand(),
                    CALLTRUE));
                conds.push_back(new Condition(filename, lineVal, DINFO));
                conds.push_back(new Condition("", NULL, HELLO));

                // Prepare function
                std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
                Type *retType = Type::getVoidTy(Ctx);
                FunctionType *funcType =
                    FunctionType::get(retType, paramTypes, false);
                FunctionCallee logFunc =
                    F.getParent()->getOrInsertFunction("_printk", funcType);

                // Prepare arguments
                std::vector<Value *> args;

                // Prepare format
                Value *format[NUM_OF_CONDTYPE];
                prepareFormat(format, builder, Ctx);

                while (!conds.empty()) {
                    Condition *cond = conds.back();
                    conds.pop_back();

                    if (cond->Type == HELLO) {
                        args.push_back(format[cond->Type]);
                        builder.CreateCall(logFunc, args);
                        DEBUG_PRINT("Inserted log for HELLO\n");
                        args.clear();
                        delete cond;
                        continue;
                    }
                    args.push_back(format[cond->Type]);
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
