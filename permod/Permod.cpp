#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/IR/DebugLoc.h"

#include "ConditionAnalysis.hpp"
#include "Instrumentation.hpp"
#include "debug.h"

using namespace llvm;

namespace permod {

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

  void printValue(Value *Parent, BasicBlock *CondBB) {
    // BB won't be a part of condition
    if (isa<BasicBlock>(Parent)) {
      return;
    }
    if (isa<Function>(Parent)) {
      DEBUG_PRINT("Should we print function here?");
      return;
    }

    Instruction *I = dyn_cast<Instruction>(Parent);
    if (!I) {
      if (isa<ConstantInt>(Parent)) {
        PRETTY_PRINT(cast<ConstantInt>(Parent)->getSExtValue());
        return;
      }
      // null pointer does not have name
      if (isa<ConstantPointerNull>(Parent)) {
        PRETTY_PRINT("NULL");
        return;
      }
      PRETTY_PRINT(Parent->getName());
      return;
    }

    if (isa<AllocaInst>(I)) {
      StringRef name = I->getName();

      // Pointer variable X passed as function arugment is loaded to X.addr.
      if (name.endswith(".addr")) {
        name = name.drop_back(5);
      }

      // The name of function argument is acceptable.
      Function *F = I->getFunction();
      for (auto &Arg : F->args()) {
        DEBUG_PRINT("Arg: " << Arg << "\n");
        if (Arg.getName().equals(name)) {
          break;
        }
      }

      PRETTY_PRINT(name);
      return;
    }

    if (isa<CallInst>(I)) {
      CallInst *CI = cast<CallInst>(I);

      if (CI->getCalledFunction() == nullptr) {
        PRETTY_PRINT("call ");
        return;
      }

      StringRef name = CI->getCalledFunction()->getName();
      if (name.empty())
        name = "NONAME FUNCTION";
      PRETTY_PRINT(name << "(");

      Value *Arg;
      for (auto &Arg : CI->args()) {
        printValue(Arg.get(), CondBB);
        if (std::next(&Arg) != CI->arg_end()) {
          PRETTY_PRINT(", ");
        }
      }

      PRETTY_PRINT(")");
      return;
    }

    // TODO: Nested struct
    // FIXME: stop using getVarName()
    if (isa<GetElementPtrInst>(I)) {
      GetElementPtrInst *GEP = cast<GetElementPtrInst>(I);
      PRETTY_PRINT(ConditionAnalysis::getStructName(*GEP));
      PRETTY_PRINT("->");
      PRETTY_PRINT(ConditionAnalysis::getVarName(*GEP));
      return;
    }

    DEBUG_PRINT2("\n[TODO] Parent: " << *Parent << "\n");

    Value *Child;

    Child = I->getOperand(0);
    DEBUG_PRINT2("Parent: " << *Parent << "\nChild: " << *Child << "\n");
    printValue(Child, CondBB);
    if (isa<CmpInst>(I)) {
      switch (cast<CmpInst>(I)->getPredicate()) {
      case CmpInst::Predicate::ICMP_EQ:
        PRETTY_PRINT(" == ");
        break;
      case CmpInst::Predicate::ICMP_NE:
        PRETTY_PRINT(" != ");
        break;
      case CmpInst::Predicate::ICMP_UGT:
      case CmpInst::Predicate::ICMP_SGT:
        PRETTY_PRINT(" > ");
        break;
      case CmpInst::Predicate::ICMP_UGE:
      case CmpInst::Predicate::ICMP_SGE:
        PRETTY_PRINT(" >= ");
        break;
      case CmpInst::Predicate::ICMP_ULT:
      case CmpInst::Predicate::ICMP_SLT:
        PRETTY_PRINT(" < ");
        break;
      case CmpInst::Predicate::ICMP_ULE:
      case CmpInst::Predicate::ICMP_SLE:
        PRETTY_PRINT(" <= ");
        break;
      default:
        PRETTY_PRINT(" cmp_" << cast<CmpInst>(I)->getPredicate() << " ");
      }
    } else if (isa<LoadInst>(I) || isa<SExtInst>(I) || isa<ZExtInst>(I) ||
               isa<TruncInst>(I)) {
      // DO NOTHING
    } else {
      PRETTY_PRINT(" " << I->getOpcodeName() << " ");
    }

    for (int i = 1; i < I->getNumOperands(); i++) {
      Child = I->getOperand(i);
      DEBUG_PRINT2("Parent: " << *Parent << "\nChild: " << *Child << "\n");
      printValue(Child, CondBB);

      if (i != I->getNumOperands() - 1) {
        PRETTY_PRINT(" ");
      }
    }
  }

  /*
   *****************
   * main function *
   *****************
   */
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
#ifndef TEST
    /* Analyze only fs/ directory */
    if (M.getName().find("/fs/") == std::string::npos) {
      DEBUG_PRINT("Skip: " << M.getName() << "\n");
      return PreservedAnalyses::all();
    }
#endif
    DEBUG_PRINT("Module: " << M.getName() << "\n");
    bool modified = false;
    for (auto &F : M.functions()) {

      /* Skip specific functions */
      if (F.isDeclaration()) {
        DEBUG_PRINT2("--- Skip Declaration\n");
        continue;
      }
      if (F.getName().startswith("llvm")) {
        DEBUG_PRINT2("--- Skip llvm\n");
        continue;
      }
      if (F.getName() == LOGGR_FUNC || F.getName() == BUFFR_FUNC ||
          F.getName() == FLUSH_FUNC) {
        DEBUG_PRINT2("--- Skip permod API\n");
        continue;
      }
      if (!F.getName().startswith("proc_lookup_de")) {
        // continue;
      }
      DEBUG_PRINT(F);

      /* Find return statement of the function */
      ReturnInst *RetI;
      for (auto &BB : F) {
        if (auto *RI = dyn_cast_or_null<ReturnInst>(BB.getTerminator())) {
          RetI = RI;
          break;
        }
      }
      if (!RetI) {
        DEBUG_PRINT2("*** No return\n");
        continue;
      }
      if (RetI->getNumOperands() == 0) {
        DEBUG_PRINT2("** Terminator " << *RetI << " has no operand\n");
        continue;
      }

      DebugInfo DBinfo;
      BasicBlock *RetBB = RetI->getParent();
      ConditionAnalysis::getDebugInfo(DBinfo, *RetI, F);

      DEBUG_PRINT2("Function:" << F << "\n");

      class Instrumentation Ins(&F);
      long long cond_num = 0;

      /* Insert logger just before terminator of every BB */
      for (BasicBlock &BB : F) {
        if (BB.getTerminator()->getNumSuccessors() <= 1) {
          continue;
        }
        DEBUG_PRINT2("Instrumenting BB: " << BB.getName() << "\n");

        PRETTY_PRINT(DBinfo.first << "::" << DBinfo.second << "()#" << cond_num
                                  << ": ");

        Instruction *term = BB.getTerminator();
        if (isa<BranchInst>(term) && term->getNumSuccessors() == 2) {
          BranchInst *BrI = cast<BranchInst>(term);
          printValue(BrI->getCondition(), &BB);
          PRETTY_PRINT("\n");
        } else if (isa<SwitchInst>(term)) {
          printValue(term, &BB);
          PRETTY_PRINT("\n");
        } else {
          DEBUG_PRINT2("Skip this terminator: " << *term << "\n");
          continue;
        }

        if (Ins.insertBufferFunc(BB, DBinfo, cond_num)) {
          modified = true;
          cond_num++;
        }
      }

      modified |= Ins.insertFlushFunc(DBinfo, *RetBB);
    }

    if (modified) {
      DEBUG_PRINT2("Modified\n");
      return PreservedAnalyses::none();
    } else {
      DEBUG_PRINT2("Not Modified\n");
      return PreservedAnalyses::all();
    }
  };
}; // namespace

} // namespace permod

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Permod pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(permod::PermodPass());
                });
          }};
}
