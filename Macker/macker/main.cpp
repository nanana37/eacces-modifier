#include "macker/LogManager.h"
#include "macker/MyASTVisitor.h"
#include "macker/MyPPCallbacks.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

namespace {

class MacroTrackerConsumer : public ASTConsumer {
private:
  MyASTVisitor visitor;
  bool enablePPCallbacks;

public:
  MacroTrackerConsumer(Rewriter &R, CompilerInstance &Instance,
                       bool EnablePPCallbacks)
      : visitor(R, Instance.getSourceManager()),
        enablePPCallbacks(EnablePPCallbacks) {

    // Only add PP callbacks if explicitly enabled
    if (enablePPCallbacks) {
      Instance.getPreprocessor().addPPCallbacks(
          std::make_unique<MyPPCallbacks>(Instance));
    }
  }

  void HandleTranslationUnit(ASTContext &Context) override {
#if defined(DEBUG)
    llvm::outs() << "Macker: Starting macro tracking...\n";
#endif
    visitor.TraverseDecl(Context.getTranslationUnitDecl());

#if defined(DEBUG)
    llvm::outs() << "Macker: Macro tracking completed.\n";
    llvm::outs() << "Macker: Writing logs to CSV file...\n";
    std::string fileName =
        Context.getSourceManager()
            .getFileEntryForID(Context.getSourceManager().getMainFileID())
            ->getName()
            .str();
    llvm::outs() << "Macker: File: " << fileName << "\n";
#endif

    // Output the logs after traversal is complete
    macker::LogManager::getInstance().writeAllLogs(true);
  }
};

class MacroTrackerAction : public PluginASTAction {
private:
  Rewriter rewriter;
  bool enablePPCallbacks;

public:
  MacroTrackerAction() : enablePPCallbacks(false) {}

protected:
  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile) override {
    return std::make_unique<MacroTrackerConsumer>(
        rewriter, CI, enablePPCallbacks);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (const auto &arg : args) {
      if (arg == "enable-pp") {
        llvm::outs() << "Macker: Preprocessing callbacks enabled\n";
        enablePPCallbacks = true;
      } else if (arg == "help") {
        llvm::errs() << "Macro Tracker Plugin Options:\n"
                     << "  enable-pp   : Enable preprocessing callbacks\n";
      }
    }
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    // NOTE: This plugin should run before the main action (e.g., compilation)
    // to ensure that logs are generated to be used by the LLVM pass.
    return AddBeforeMainAction;
  }

  void ExecuteAction() override { PluginASTAction::ExecuteAction(); }
};
} // namespace

static FrontendPluginRegistry::Add<MacroTrackerAction>
    X("macro_tracker", "Trace macro with PPCallbacks");
