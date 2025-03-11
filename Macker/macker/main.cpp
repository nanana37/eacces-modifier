#include "MyPPCallbacks.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

namespace {

class MacroTrackerConsumer : public ASTConsumer {
 CompilerInstance &Instance;

public:
  MacroTrackerConsumer(CompilerInstance &Instance) : Instance(Instance) {
    Instance.getPreprocessor().addPPCallbacks(std::make_unique<MyPPCallbacks>(Instance));
  }
};

class MyPPCallbacksAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile) override {
    return std::make_unique<MacroTrackerConsumer>(CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  // Enable this plugin to run after the main action
  // i.e., same as -add-plugin
  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }

  void ExecuteAction() override { PluginASTAction::ExecuteAction(); }
};
} // namespace

static FrontendPluginRegistry::Add<MyPPCallbacksAction>
    X("macro-tracker", "Trace macro with PPCallbacks");
