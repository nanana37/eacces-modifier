#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "ConditionAnalysis.hpp"
#include "Instrumentation.hpp"
#include "debug.h"
#include "LogManager.h"

using namespace llvm;
using namespace permod;

namespace {

/*
 * Find 'return -ERRNO'
    * The statement turns into:
    Error-thrower BB:
        store i32 -13，ptr %1, align 4
    Terminator BB:
        %2 = load i32, ptr %1, align 4
        ret i32 %2
 */
struct PermodPass : public PassInfoMixin<PermodPass> {
  // Use a pointer instead of direct member
  std::unique_ptr<ConditionPrinter> Printer;
  
  // Constructor initializes the pointer
  PermodPass() : Printer(std::make_unique<ConditionPrinter>()) {}
  
  // Now these can be defaulted
  PermodPass(PermodPass&&) = default;
  PermodPass& operator=(PermodPass&&) = default;
  
  // Break down printValue into smaller methods
  void printValue(Value *V, BasicBlock *CondBB) {
    if (isa<BasicBlock>(V) || isa<Function>(V))
      return;
    
    if (Instruction *I = dyn_cast<Instruction>(V)) {
      printInstruction(I, CondBB);
    } else {
      printConstantOrArgument(V);
      return;
    }
  }
  
  void printConstantOrArgument(Value *V) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
      *Printer << CI->getSExtValue();  // Use *Printer to dereference
    } else if (isa<ConstantPointerNull>(V)) {
      *Printer << "NULL";
    } else {
      *Printer << V->getName();
    }
  }
  
  void printInstruction(Instruction *I, BasicBlock *CondBB) {
    if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
      printAlloca(AI);
    } else if (CallInst *CI = dyn_cast<CallInst>(I)) {
      printCall(CI, CondBB);
    } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I)) {
      printGEP(GEP);
    } else {
      printGenericInstruction(I, CondBB);
    }
  }
  
  void printAlloca(AllocaInst *AI) {
    StringRef name = AI->getName();

    // Pointer variable X passed as function arugment is loaded to X.addr.
    if (name.endswith(".addr")) {
      name = name.drop_back(5);
    }

    // The name of the function's argument is acceptable to be printed.
    Function *F = AI->getFunction();
    for (auto &Arg : F->args()) {
      DEBUG_PRINT2("Arg: " << Arg << "\n");
      if (Arg.getName().equals(name)) {
        break;
      }
    }

    *Printer << name;
  }

  void printCall(CallInst *CI, BasicBlock *CondBB) {
    if (CI->getCalledFunction() == nullptr) {
      *Printer << "call ";
      return;
    }

    StringRef name = CI->getCalledFunction()->getName();
    if (name.empty())
      name = "NONAME FUNCTION";
    *Printer << name << "(";

    Value *Arg;
    for (auto &Arg : CI->args()) {
      printValue(Arg.get(), CondBB);
      if (std::next(&Arg) != CI->arg_end()) {
        *Printer << ", ";
      }
    }

    *Printer << ")";
  }

  void printGEP(GetElementPtrInst *GEP) {
    *Printer << ConditionAnalysis::getStructName(*GEP);
    *Printer << "->";
    *Printer << ConditionAnalysis::getVarName(*GEP);
  }

  void printGenericInstruction(Instruction *I, BasicBlock *CondBB) {
    DEBUG_PRINT2("\n[TODO] Parent: " << *I << "\n");

    Value *Child;

    Child = I->getOperand(0);
    DEBUG_PRINT2("Parent: " << *I << "\nChild: " << *Child << "\n");
    printValue(Child, CondBB);
    if (isa<CmpInst>(I)) {
      switch (cast<CmpInst>(I)->getPredicate()) {
      case CmpInst::Predicate::ICMP_EQ:
        *Printer << " == ";
        break;
      case CmpInst::Predicate::ICMP_NE:
        *Printer << " != ";
        break;
      case CmpInst::Predicate::ICMP_UGT:
      case CmpInst::Predicate::ICMP_SGT:
        *Printer << " > ";
        break;
      case CmpInst::Predicate::ICMP_UGE:
      case CmpInst::Predicate::ICMP_SGE:
        *Printer << " >= ";
        break;
      case CmpInst::Predicate::ICMP_ULT:
      case CmpInst::Predicate::ICMP_SLT:
        *Printer << " < ";
        break;
      case CmpInst::Predicate::ICMP_ULE:
      case CmpInst::Predicate::ICMP_SLE:
        *Printer << " <= ";
        break;
      default:
        *Printer << " cmp_" << cast<CmpInst>(I)->getPredicate() << " ";
      }
    } else if (isa<LoadInst>(I) || isa<SExtInst>(I) || isa<ZExtInst>(I) ||
               isa<TruncInst>(I)) {
      // DO NOTHING
    } else {
      *Printer << " " << I->getOpcodeName() << " ";
    }

    for (int i = 1; i < I->getNumOperands(); i++) {
      Child = I->getOperand(i);
      DEBUG_PRINT2("Parent: " << *I << "\nChild: " << *Child << "\n");
      printValue(Child, CondBB);

      if (i != I->getNumOperands() - 1) {
        *Printer << " ";
      }
    }
  }
  bool analyzeFunction(Function &F) {
    // Extract debug info
    DebugInfo DBinfo;
    ReturnInst *RetI = findReturnInst(F);
    if (!RetI) return false;
    
    ConditionAnalysis::getDebugInfo(DBinfo, *RetI, F);
    
    // Perform instrumentation
    Instrumentation Ins(&F);
    long long CondID = 0;
    
    for (BasicBlock &BB : F) {
      if (BB.getTerminator()->getNumSuccessors() <= 1)
        continue;
        
      Printer->clear();
      *Printer << DBinfo.first << "::" << DBinfo.second << "()#" << CondID << ": ";
      
      Instruction *Term = BB.getTerminator();
      std::string CondType;
      
      if (BranchInst *BrI = dyn_cast<BranchInst>(Term)) {
        if (Term->getNumSuccessors() == 2) {
          printValue(BrI->getCondition(), &BB);
          CondType = "if";
        }
      } else if (SwitchInst *SI = dyn_cast<SwitchInst>(Term)) {
        printValue(Term, &BB);
        CondType = "switch";
      } else {
        continue;
      }
      
      unsigned LineNum = 0;
      if (DebugLoc DL = Term->getDebugLoc()) {
        LineNum = DL.getLine();
      }

      LogManager::getInstance().addEntry(
        DBinfo.first, LineNum, DBinfo.second, CondType, CondID, Printer->str());
        
      // Add instrumentation
      if (Ins.insertBufferFunc(BB, DBinfo, CondID)) {
        CondID++;
      }
    }
    
    return Ins.insertFlushFunc(DBinfo, *RetI->getParent());
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    // Filter modules
    if (!shouldProcessModule(M))
      return PreservedAnalyses::all();
      
    bool Modified = false;
    for (auto &F : M.functions()) {
      if (shouldProcessFunction(F)) {
        Modified |= analyzeFunction(F);
      }
    }
    
    // Write all logs to a CSV file
    std::error_code EC;
    llvm::raw_fd_ostream OS("permod_conditions.csv", EC);
    if (!EC) {
      LogManager::getInstance().writeAllLogs(true);
    }
    
    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
  
  // Helper methods
  bool shouldProcessModule(Module &M) {
#ifndef TEST
    return M.getName().find("/fs/") != std::string::npos;
#else
    return true;
#endif
  }
  
  bool shouldProcessFunction(Function &F) {
    return !F.isDeclaration() && 
           !F.getName().startswith("llvm") &&
           F.getName() != LOGGR_FUNC && 
           F.getName() != BUFFR_FUNC &&
           F.getName() != FLUSH_FUNC;
  }
  
  ReturnInst *findReturnInst(Function &F) {
    for (auto &BB : F) {
      if (auto *RI = dyn_cast_or_null<ReturnInst>(BB.getTerminator())) {
        return RI;
      }
    }
    return nullptr;
  }
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
