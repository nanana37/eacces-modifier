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
    * NOTE: This is recursive!
    */
    Value *getOrigin(Value *V) {
        Value *Prev = V;

        DEBUG_PRINT("Getting origin of: " << *V << "\n");

        for (int i = 0; i < MAX_TRACE_DEPTH; i++) {

            if (auto *LI = dyn_cast<LoadInst>(V)) {
                V = LI->getPointerOperand();
                DEBUG_PRINT("V was Load: " << *V << "\n");
            }

            if (auto *AI = dyn_cast<BinaryOperator>(V)) {
                V = AI->getOperand(0);
                DEBUG_PRINT("V was And: " << *V << "\n");
            }

            // #1: store i32 %flag, ptr %flag.addr, align 4
            for (User *U : V->users()) {
                StoreInst *SI = dyn_cast<StoreInst>(U);
                if (!SI)
                    continue;
                V = SI->getValueOperand(); // %flag
                DEBUG_PRINT("V was Store: " << *V << "\n");
            }

            // NOTE: Special case for kernel
            if (auto *ZI = dyn_cast<ZExtInst>(V)) {
                V = ZI->getOperand(0);
                DEBUG_PRINT("V was ZExt: " << *V << "\n");
                if (auto *ZLI = dyn_cast<LoadInst>(V)) {
                    V = ZLI->getOperand(0);
                    DEBUG_PRINT("V was ZExt Load: " << *V << "\n");
                }
            }

            // GetElementPtrInst: Get struct from member
            // TODO?: Is this necessary?
            if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
                /* V = GEP->getPointerOperand(); */
                DEBUG_PRINT("V was GEP: " << *V << "\n");
                return V;
            }

            // Stop the recursion
            if (!isa<Instruction>(V)) {
                DEBUG_PRINT("Origin: " << *V << "\n");
                return V;
            }
            if (isa<CallInst>(V)) {
                DEBUG_PRINT("Origin: " << *V << "\n");
                V = dyn_cast<CallInst>(V)->getCalledFunction();
                return V;
            }
            if (isa<SExtInst>(V)) {
                DEBUG_PRINT("Origin: " << *V << "\n");
                V = dyn_cast<SExtInst>(V)->getOperand(0);
                return V;
            }
            if (isa<AllocaInst>(V)) {
                DEBUG_PRINT("Origin: " << *V << "\n");
                return V;
            }
            if (V == Prev) {
                DEBUG_PRINT("** Cannot find origin anymore: " << *V << "\n");
                return V;
            }
        }

        DEBUG_PRINT("** Too deep recursion: " << *V << "\n");
        return V;
    }

    /*
     * Get struct name, if the V is a member of struct
     */
    StringRef getStructName(Value *V) {
        if (!isa<GetElementPtrInst>(V))
            return "";
        Value *PtrOp = dyn_cast<GetElementPtrInst>(V)->getPointerOperand();
        if (!PtrOp)
            return "";
        DEBUG_PRINT("PtrOp: " << *PtrOp << "\n");
        return getVarName(PtrOp);
    }

    /*
     * Get variable name
     */
    StringRef getVarName(Value *V) {
        StringRef Name = getOrigin(V)->getName();
        if (Name.empty())
            Name = "Unnamed Condition";
        /* StringRef StructName = getStructName(V); */
        /* if (!StructName.empty()) */
        /*     Name = Twine(StructName) + "." + Twine(Name); */
        DEBUG_PRINT("Name: " << Name << "\n");
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
            LoadInst *LI = dyn_cast<LoadInst>(RetVal);
            if (!LI) {
                /* DEBUG_PRINT("RetVal" << *RetVal << "\n"); */
                /* DEBUG_PRINT("~~~ Return value is not LoadInst\n"); */
                continue;
            }
            RetVal = LI->getPointerOperand();

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
        CMPNULLTRUE,
        CMPNULLFALSE,
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
        /* DEBUG_PRINT(*CmpI->getParent() << "\n"); */

        Value *CmpOp2 = CmpI->getOperand(1);
        if (!CmpOp2)
            return nullptr;
        /* if cmp with null
         e.g. if (!inode):
            %tobool = icmp ne %inode, null
            br i1 %tobool, label %if.end, label %if.then
         */
        if (isa<ConstantPointerNull>(CmpOp2)) {
            DEBUG_PRINT("CmpOp2 is null\n");
            name = getVarName(CmpOp);
            val = CmpOp2;
            type = isBranchTrue(BrI, DestBB) ? CMPNULLFALSE : CMPNULLTRUE;
            if (type == CMPNULLTRUE)
                DEBUG_PRINT("Branch is true\n");
            else
                DEBUG_PRINT("Branch is false\n");
            return new Condition(name, val, type);
        }

        // CmpOp: %and = and i32 %flag, 2
        if (auto *AndI = dyn_cast<BinaryOperator>(CmpOp)) {
            DEBUG_PRINT("AndI as CmpOp: " << *AndI << "\n");
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
            DEBUG_PRINT("************FIND ME**************\n");
            DEBUG_PRINT("Parent: " << *CmpI->getParent() << "\n");
            DEBUG_PRINT("CallI as CmpOp: " << *CallI << "\n");
            Function *Callee = CallI->getCalledFunction();
            if (!Callee)
                return nullptr;

            name = getVarName(Callee);
            val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), 0);
            type = isBranchTrue(BrI, DestBB) ? CALLTRUE : CALLFALSE;
            if (type == CALLTRUE)
                DEBUG_PRINT("Branch is true\n");
            else
                DEBUG_PRINT("Branch is false\n");
            return new Condition(name, val, type);
        }

        // CmpOp: %1 = load i32, i32* %flag.addr, align 4
        // TODO: Try on some examples
        if (auto *LoadI = dyn_cast<LoadInst>(CmpOp)) {
            DEBUG_PRINT("LoadI as CmpOp: " << *LoadI << "\n");
            name = getVarName(LoadI);
            val = CmpI->getOperand(1);

            type = isBranchTrue(BrI, DestBB) ? CMPTRUE : CMPFALSE;
            DEBUG_PRINT("name: " << name << "\n");
            DEBUG_PRINT("val: " << *val << "\n");
            DEBUG_PRINT("type: " << type << "\n");
            return new Condition(name, val, type);
        }

        DEBUG_PRINT("** Unexpected as CmpOp: " << *CmpOp << "\n");
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


        DEBUG_PRINT("BrI: " << *BrI << "\n");
        DEBUG_PRINT("CallI: " << *CallI << "\n");
        DEBUG_PRINT("name of DestBB: " << DestBB->getName() << "\n");

        name = getVarName(Callee);
        val = ConstantInt::get(Type::getInt32Ty(CallI->getContext()), 0);
        type = isBranchTrue(BrI, DestBB) ? CALLTRUE : CALLFALSE;
        if (type == CALLTRUE)
            DEBUG_PRINT("Branch is true\n");
        else
            DEBUG_PRINT("Branch is false\n");
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
            DEBUG_PRINT("Parent: " << *BrI->getParent() << "\n");
            return getIfCond_cmp(BrI, dyn_cast<CmpInst>(IfCond), DestBB);
        }
        // Call condition: if (!func()) {}
        if (isa<CallInst>(IfCond)) {
            DEBUG_PRINT("IfCond is Call: " << *IfCond << "\n");
            DEBUG_PRINT("Parent: " << *BrI->getParent() << "\n");
            return getIfCond_call(BrI, dyn_cast<CallInst>(IfCond), DestBB);
        }
        DEBUG_PRINT("** Unexpected as IfCond: " << *IfCond << "\n");
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
        /* DEBUG_PRINT("Search preds of: " << *BB << "\n"); */
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
            DEBUG_PRINT("OMG");
            DEBUG_PRINT("**************PredBB terminator is not a branch or switch\n");
            DEBUG_PRINT("**************PredBB: " << *PredBB << "\n");
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
                DEBUG_PRINT("*OMGOMGOMGOMG No CondBB in preds\n");
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
            DEBUG_PRINT("Added condition: " << cond->Name << "\n\n");

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
#ifdef DEBUG
        formatStr[CMPTRUE] = "[Permod] %s: %d\n";
        formatStr[CMPFALSE] = "[Permod] %s: not %d\n";
        formatStr[CMPNULLTRUE] = "[Permod] %s: null\n";
        formatStr[CMPNULLFALSE] = "[Permod] %s: not null\n";
        formatStr[CALLTRUE] = "[Permod] %s(): not %d\n";
        formatStr[CALLFALSE] = "[Permod] %s(): %d\n";
        formatStr[SWITCH] = "[Permod] %s: %d (switch)\n";
        formatStr[DINFO] = "[Permod] %s: %d\n";
        formatStr[HELLO] = "--- Hello, I'm Permod ---\n";
#else
        formatStr[CMPTRUE] = " %s: %d\n";
        formatStr[CMPFALSE] = " %s: not %d\n";
        formatStr[CMPNULLTRUE] = " %s: null\n";
        formatStr[CMPNULLFALSE] = " %s: not null\n";
        formatStr[CALLTRUE] = " %s(): not 0\n";
        formatStr[CALLFALSE] = " %s(): 0\n";
        formatStr[SWITCH] = " %s: %d (switch)\n";
        formatStr[DINFO] = " %s: %d\n";
        formatStr[HELLO] = "--- Hello, I'm Permod ---\n";
#endif // DEBUG

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

            // Skip
            if (F.getName() == "_printk")
                continue;

            Value *RetVal = getReturnValue(&F);
            if (!RetVal)
                continue;
            /* DEBUG_PRINT("Return value: " << *RetVal << "\n\n"); */

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
                // The analyzing function
                conds.push_back(new Condition(
                    F.getName(), dyn_cast<StoreInst>(U)->getValueOperand(),
                    CALLFALSE));
                // File name and line number
                conds.push_back(new Condition(filename, lineVal, DINFO));
                // Hello
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

                    // Check if the Value is ConstantPointerNull
                    if (cond->Val && isa<ConstantPointerNull>(cond->Val)) {
                        DEBUG_PRINT("Val is ConstantPointerNull\n");
                        if (cond->Type == CMPTRUE)
                            cond->Type = CMPNULLTRUE;  // maybe never reached
                        else if (cond->Type == CMPFALSE)
                            cond->Type = CMPNULLFALSE;  // maybe never reached
                        else
                            DEBUG_PRINT("** Unexpected type:" << cond->Type <<"\n");
                    }

                    // Insert log
                    args.push_back(format[cond->Type]);
                    args.push_back(builder.CreateGlobalStringPtr(cond->Name));
                    args.push_back(cond->Val);
                    builder.CreateCall(logFunc, args);
                    /* DEBUG_PRINT("Inserted log for " << cond->Name << "\n"); */
                    /* DEBUG_PRINT("Val: " << *cond->Val << "\n"); */
                    /* DEBUG_PRINT("Type: " << cond->Type << "\n"); */
#ifdef DEBUG
                    DEBUG_PRINT( cond->Name << ": " << *cond->Val );
                    switch (cond->Type) {
                        case CMPTRUE:
                            DEBUG_PRINT(" CMPTRUE\n");
                            break;
                        case CMPFALSE:
                            DEBUG_PRINT(" CMPFALSE\n");
                            break;
                        case CMPNULLTRUE:
                            DEBUG_PRINT(" CMPNULLTRUE\n");
                            break;
                        case CMPNULLFALSE:
                            DEBUG_PRINT(" CMPNULLFALSE\n");
                            break;
                        case CALLTRUE:
                            DEBUG_PRINT(" CALLTRUE\n");
                            break;
                        case CALLFALSE:
                            DEBUG_PRINT(" CALLFALSE\n");
                            break;
                        case SWITCH:
                            DEBUG_PRINT(" SWITCH\n");
                            break;
                        case DINFO:
                            DEBUG_PRINT(" DINFO\n");
                            break;
                        case HELLO:
                            DEBUG_PRINT(" HELLO\n");
                            break;
                        default:
                            DEBUG_PRINT(" " << cond->Type << "\n");
                            break;
                    }
#endif // DEBUG


                    args.clear();

                    delete cond;
                }
                if (conds.empty()) {
                    DEBUG_PRINT("~~~ Inserted all logs ~~~\n");
                } else {
                    DEBUG_PRINT("** Failed to insert all logs\n");
                    deleteAllCond(conds);
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
