#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "macker/LogManager.h"
#include "permod/ConditionAnalysis.hpp"
#include "permod/Instrumentation.hpp"
#include "permod/LogManager.h"
#include "permod/LogParser.h"
#include "utils/debug.h"

using namespace llvm;

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
  PermodPass(PermodPass &&) = default;
  PermodPass &operator=(PermodPass &&) = default;

  // Break down printValue into smaller methods
  void printValue(Value *V, BasicBlock *CondBB) {
    if (isa<BasicBlock>(V) || isa<Function>(V))
      return;

    DEBUG_PRINT("printValue: " << *V << "\n");
    if (Instruction *I = dyn_cast<Instruction>(V)) {
      printInstruction(I, CondBB);
    } else {
      // printConstantOrArgument(V);
      return;
    }
  }

  void printConstantOrArgument(Value *V) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
      *Printer << CI->getSExtValue(); // Use *Printer to dereference
    } else if (isa<ConstantPointerNull>(V)) {
      *Printer << "NULL";
    } else {
      *Printer << V->getName();
    }
  }

  void printInstruction(Instruction *I, BasicBlock *CondBB) {
    DEBUG_PRINT("printInstruction: " << *I << "\n");
    if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
      // printAlloca(AI);
    } else if (CallInst *CI = dyn_cast<CallInst>(I)) {
      printCall(CI, CondBB);
    } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I)) {
      printGEP(GEP);
    } else {
      printGenericInstruction(I, CondBB);
    }
  }

  void printAlloca(AllocaInst *AI) {
    DEBUG_PRINT("printAlloca: " << *AI << "\n");
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

  Value *getLatestValue(AllocaInst *AI, BasicBlock *TheBB) {
    if (!AI)
      return nullptr;
    // Search for the latest value of the AllocaInst
    Value *val = nullptr;
    for (auto &I : *TheBB) {
      if (auto *StoreI = dyn_cast<StoreInst>(&I)) {
        if (StoreI->getPointerOperand() == AI) {
          val = StoreI->getValueOperand();
          break;
        }
      }
    }
    return val;
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
    for (int i = 0; i < I->getNumOperands(); i++) {
      Child = I->getOperand(i);
      if (auto val = getLatestValue(dyn_cast<AllocaInst>(Child), CondBB)) {
        if (!isa<Instruction>(val)) {
          return;
        }
        *Printer << " (";
        if (auto *I = dyn_cast<Instruction>(val)) {
          if (I->getDebugLoc()) {
            DEBUG_PRINT("Line: " << I->getDebugLoc().getLine() << "\n");
            *Printer << "[#" << I->getDebugLoc().getLine() << "] ";
          }
        }
        if (isa<CallInst>(val)) {
          Function *F = cast<CallInst>(val)->getCalledFunction();
          if (F) {
            *Printer << Child->getName() << " = " << F->getName() << "()";
          } else {
            *Printer << "NONAME FUNCTION";
          }
        }
        *Printer << ")";
      }
      printValue(Child, CondBB);
    }
    return;

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
  bool
  analyzeFunction(Function &F,
                  std::vector<macker::LogManager::LogEntry> &FunctionLogs) {
    // Extract debug info
    DebugInfo DBinfo;
    ReturnInst *RetI = findReturnInst(F);
    if (!RetI)
      return false;

    ConditionAnalysis::getDebugInfo(DBinfo, *RetI, F);

#if defined(KERNEL_MODE) && defined(DEBUG2)
    // Print IR
    if (F.getName() == "acl_permission_check") {
      // if (F.getName() == "may_open") {
      DEBUG_PRINT("Function: " << F.getName() << "\n");
      DEBUG_PRINT2(F << "\n");
    } else {
      return false;
    }
#endif

    // Perform instrumentation
    Instrumentation Ins(&F);
    long long CondID = 0;

    for (BasicBlock &BB : F) {
      Instruction *Term = BB.getTerminator();
      if (Term->getNumSuccessors() <= 1)
        continue;
      if (!Term->getDebugLoc())
        continue;
      // Skip if the terminator is generated by the compiler
      if (Term->getMetadata("nosanitize"))
        continue;

      // print the terminator instruction
      DEBUG_PRINT2("Term: " << *Term << "\n");
      DEBUG_PRINT2("(LLVM) Line: " << (Term->getDebugLoc()).getLine() << "\n");

      // Check if the terminator of the current block has been logged
      const macker::LogManager::LogEntry *mackerLogEntry = nullptr;
      for (const auto &log : FunctionLogs) {
        // Skip "case" log entries
        // TODO: reffer to them when analyzing "switch"
        if ("case" == log.EventType) {
          DEBUG_PRINT2("Found case log entry: " << log.Content << "\n");
          continue;
        }
        // print log entry
        DEBUG_PRINT2("Log: " << log.Content << "\n");
        DEBUG_PRINT2("File: " << log.FileName << "\n");
        DEBUG_PRINT2("(Clang) Line: " << log.LineNumber << "\n");
        if (log.LineNumber == (Term->getDebugLoc()).getLine()) {
          DEBUG_PRINT2("Found macker log entry: " << log.Content << "\n");
          mackerLogEntry = &log;
          break;
        }
      }
      if (!mackerLogEntry) {
        DEBUG_PRINT2("No macker log entry found\n");
        continue;
      }
      DEBUG_PRINT2("Found macker log entry: " << mackerLogEntry->Content
                                              << "\n");

      Printer->clear();
      *Printer << mackerLogEntry->Content;

      std::string CondType;

      if (BranchInst *BrI = dyn_cast<BranchInst>(Term)) {
        if (Term->getNumSuccessors() == 2) {
          printValue(BrI->getCondition(), &BB);
          CondType = "if";
        }
      } else if (SwitchInst *SI = dyn_cast<SwitchInst>(Term)) {
        CondType = "switch";
      } else {
        continue;
      }

      unsigned LineNum = 0;
      if (DebugLoc DL = Term->getDebugLoc()) {
        LineNum = DL.getLine();
      }

#if defined(DEBUG2)
      DEBUG_PRINT("******\n");
      DEBUG_PRINT("Adding entry to LogManager\n");
      DEBUG_PRINT("DBinfo: " << DBinfo.first << ", " << DBinfo.second << "\n");
      DEBUG_PRINT("LineNum: " << LineNum << "\n");
      DEBUG_PRINT2("CondType: " << CondType << "\n");
      DEBUG_PRINT2("CondID: " << CondID << "\n");
      DEBUG_PRINT("Printer: " << Printer->str() << "\n");
      DEBUG_PRINT(BB << "\n");
      DEBUG_PRINT("******\n");
#endif
      LogManager::getInstance().addEntry(DBinfo.first,
                                         LineNum,
                                         DBinfo.second,
                                         CondType,
                                         CondID,
                                         Printer->str());

      // Add instrumentation
      if (Ins.insertBufferFunc(BB, DBinfo, CondID)) {
        CondID++;
        DEBUG_PRINT2("Inserted at " << BB.getName() << "\n");
        DEBUG_PRINT2(BB << "\n");
      }
    }

    return Ins.insertFlushFunc(DBinfo, *RetI->getParent());
  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    // Filter modules
    if (!shouldProcessModule(M))
      return PreservedAnalyses::all();

    // Parse Macker Logs
    LogParser parser("macker_logs.csv");
    parser.parse();

    const auto &logs = parser.getParsedLogs();
#if defined(DEBUG2)
    // Print parsed logs for debugging
    for (const auto &log : logs) {
      llvm::outs() << "File: " << log.FileName << ", Line: " << log.LineNumber
                   << ", Function: " << log.FunctionName
                   << ", Event: " << log.EventType
                   << ", Content: " << log.Content
                   << ", Extra: " << log.ExtraInfo << "\n";
    }
#endif

    bool Modified = false;
    for (auto &F : M.functions()) {
      if (shouldProcessFunction(F)) {
        // Filter logs for the current function
        std::vector<macker::LogManager::LogEntry> FunctionLogs;
        for (const auto &log : logs) {
          if (log.FunctionName == F.getName()) {
            FunctionLogs.push_back(log);
          }
        }
#if defined(DEBUG2)
        // print the logs for the current function
        for (const auto &log : FunctionLogs) {
          llvm::outs() << "File: " << log.FileName
                       << ", Line: " << log.LineNumber
                       << ", Function: " << log.FunctionName
                       << ", Event: " << log.EventType
                       << ", Content: " << log.Content
                       << ", Extra: " << log.ExtraInfo << "\n";
        }
#endif

        // Process the function with its logs
        Modified |= analyzeFunction(F, FunctionLogs);
      }
    }

    DEBUG_PRINT2("Permod: Finished analyzing functions.\n");
    DEBUG_PRINT2("Permod: Writing logs to CSV file...\n");
    DEBUG_PRINT2("Permod: File: " << M.getName() << "\n");
    // Write all logs to a CSV file
    LogManager::getInstance().writeAllLogs();

    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  // Helper methods
  bool shouldProcessModule(Module &M) {
#if defined(KERNEL_MODE)
    return M.getName().find("fs/") != std::string::npos;
#else
    return true;
#endif
  }

  bool shouldProcessFunction(Function &F) {
    // clang-format off
    return !F.isDeclaration() && 
           !F.getName().startswith("llvm") &&
           F.getName() != LOGGR_FUNC && 
           F.getName() != BUFFR_FUNC &&
           F.getName() != FLUSH_FUNC;
    // clang-format on
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
