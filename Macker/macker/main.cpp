#include "LogManager.h"
#include "MyASTVisitor.h"
#include "MyPPCallbacks.h"
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
    visitor.TraverseDecl(Context.getTranslationUnitDecl());

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
      if (arg == "-enable-pp" || arg == "--enable-pp") {
        enablePPCallbacks = true;
      } else if (arg == "-help" || arg == "--help") {
        llvm::errs()
            << "Macro Tracker Plugin Options:\n"
            << "  -enable-pp, --enable-pp   : Enable preprocessing callbacks\n";
      }
    }
    return true;
  }

  // Enable this plugin to run after the main action
  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }

  void ExecuteAction() override { PluginASTAction::ExecuteAction(); }
};
} // namespace

static FrontendPluginRegistry::Add<MacroTrackerAction>
    X("macro-tracker", "Trace macro with PPCallbacks");
