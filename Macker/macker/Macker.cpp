//===- Clang Plugin: If Condition Variable Tracer ------------------------===//
// Purpose: Traces the source definitions of variables used in if-statement
//          conditions. Also records macro function argument usage for
//          completeness.
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;

namespace {

struct MacroCallInfo {
  std::string macroName;
  unsigned callLine;
  std::vector<std::string> argTexts;
};

static std::vector<MacroCallInfo> MacroCalls;

class MyPPCallbacks : public PPCallbacks {
public:
  MyPPCallbacks(SourceManager &SM) : SM(SM) {}

  void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                    SourceRange Range, const MacroArgs *Args) override {
    std::string macroName = MacroNameTok.getIdentifierInfo()->getName().str();
    unsigned line = SM.getSpellingLineNumber(MacroNameTok.getLocation());

    MacroCallInfo info;
    info.macroName = macroName;
    info.callLine = line;

    if (Args) {
      for (unsigned i = 0; i < Args->getNumMacroArguments(); ++i) {
        const Token *tokenPtr = Args->getUnexpArgument(i);
        ArrayRef<Token> tokens(tokenPtr, Args->getArgLength(tokenPtr));

        std::string argText;
        for (const Token &token : tokens)
          argText += Lexer::getSpelling(token, SM, LangOptions());

        info.argTexts.push_back(argText);
      }
    }

    MacroCalls.push_back(std::move(info));
  }

private:
  SourceManager &SM;
};

class MyVisitor : public RecursiveASTVisitor<MyVisitor> {
public:
  explicit MyVisitor(ASTContext &Context) : Context(Context) {}

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
    if (!VD)
      return true;

    std::string varName = VD->getNameAsString();
    SourceManager &SM = Context.getSourceManager();
    unsigned useLine = SM.getSpellingLineNumber(DRE->getExprLoc());

    for (const auto &call : MacroCalls) {
      if (std::abs((int)useLine - (int)call.callLine) <= 1) {
        if (std::find(call.argTexts.begin(), call.argTexts.end(), varName) !=
            call.argTexts.end()) {
          unsigned defLine = SM.getSpellingLineNumber(VD->getLocation());
          llvm::errs() << "[Macro Arg] " << varName << " used in macro "
                       << call.macroName << ", defined at line " << defLine
                       << "\n";
        }
      }
    }
    return true;
  }

  bool VisitIfStmt(IfStmt *stmt) {
    Expr *cond = stmt->getCond();
    unsigned line =
        Context.getSourceManager().getSpellingLineNumber(stmt->getIfLoc());
    llvm::errs() << "[If] condition at line " << line << "\n";
    traceExpr(cond);
    return true;
  }

  void traceExpr(Expr *expr) {
    if (!expr)
      return;
    expr = expr->IgnoreParenImpCasts();

    if (auto *DRE = dyn_cast<DeclRefExpr>(expr)) {
      auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
      if (!VD)
        return;

      SourceManager &SM = Context.getSourceManager();
      unsigned defLine = SM.getSpellingLineNumber(VD->getLocation());
      llvm::errs() << "  Var " << VD->getNameAsString() << " defined at line "
                   << defLine << "\n";

      if (VD->hasInit()) {
        traceExpr(VD->getInit());
      }
    } else if (auto *BO = dyn_cast<BinaryOperator>(expr)) {
      traceExpr(BO->getLHS());
      traceExpr(BO->getRHS());
    } else if (auto *CE = dyn_cast<CallExpr>(expr)) {
      for (Expr *arg : CE->arguments())
        traceExpr(arg);
    }
  }

private:
  ASTContext &Context;
};

class MyASTConsumer : public ASTConsumer {
public:
  explicit MyASTConsumer(ASTContext &Context) : Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  MyVisitor Visitor;
};

class IfConditionTrackerAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    CI.getPreprocessor().addPPCallbacks(
        std::make_unique<MyPPCallbacks>(CI.getSourceManager()));

    return std::make_unique<MyASTConsumer>(CI.getASTContext());
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &) override {
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddAfterMainAction;
  }

  void ExecuteAction() override { PluginASTAction::ExecuteAction(); }
};

} // namespace

static FrontendPluginRegistry::Add<IfConditionTrackerAction>
    X("if-cond-tracker",
      "Trace definitions of variables used in if-statement conditions");