#include "MyASTVisitor.h"
#include "MyPPCallbacks.h"
#include "Utilities.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

namespace {

class MacroTrackerConsumer : public ASTConsumer {
private:
  MyASTVisitor visitor;

public:
  MacroTrackerConsumer(Rewriter &R, CompilerInstance &Instance) : visitor(R, Instance.getSourceManager()) {
    Instance.getPreprocessor().addPPCallbacks(std::make_unique<MyPPCallbacks>(Instance));
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

protected:
  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile) override {
    return std::make_unique<MacroTrackerConsumer>(rewriter, CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  // Enable this plugin to run after the main action
  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }

  void ExecuteAction() override { 
    PluginASTAction::ExecuteAction();
  }
};
} // namespace

static FrontendPluginRegistry::Add<MacroTrackerAction>
    X("macro-tracker", "Trace macro with PPCallbacks");
