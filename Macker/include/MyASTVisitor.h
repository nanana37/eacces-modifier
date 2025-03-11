#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  explicit MyASTVisitor(Rewriter &R, SourceManager &SM)
      : rewriter(R), srcManager(SM), CurrentFunction(nullptr) {}

  bool VisitFunctionDecl(FunctionDecl *Func) {
    if (Func->hasBody()) {
      CurrentFunction = Func;
    }
    return true;
  }

  bool VisitIfStmt(IfStmt *If) {
    Expr *Cond = If->getCond();
    if (!Cond)
      return true;

    std::string FuncName = getCurrentFunctionName();
    llvm::outs() << "In function: " << FuncName << "\n";

    analyzeCondition(Cond);

    return true;
  }

  bool VisitSwitchStmt(SwitchStmt *Switch) {
    Expr *Cond = Switch->getCond();
    if (!Cond)
      return true;

    std::string FuncName = getCurrentFunctionName();
    llvm::outs() << "In function: " << FuncName << "\n";

    SourceRange CondRange = Cond->getSourceRange();
    std::string CondText = getSourceText(CondRange);

    const FileEntry *fileEntry = srcManager.getFileEntryForID(
        srcManager.getFileID(CondRange.getBegin()));
    unsigned int line = srcManager.getSpellingLineNumber(CondRange.getBegin());
    if (fileEntry)
      llvm::outs() << fileEntry->getName() << ":" << line << "\n";

    llvm::outs() << "Switch condition: " << CondText << "\n";

    return true;
  }

  bool VisitCaseStmt(CaseStmt *Case) {
    Expr *LHS = Case->getLHS();
    if (!LHS)
      return true;

    std::string FuncName = getCurrentFunctionName();
    llvm::outs() << "In function: " << FuncName << "\n";

    SourceRange CaseRange = LHS->getSourceRange();
    std::string CaseText = getSourceText(CaseRange);

    const FileEntry *fileEntry = srcManager.getFileEntryForID(
        srcManager.getFileID(CaseRange.getBegin()));
    unsigned int line = srcManager.getSpellingLineNumber(CaseRange.getBegin());
    if (fileEntry)
      llvm::outs() << fileEntry->getName() << ":" << line << "\n";

    llvm::outs() << "Case value: " << CaseText << "\n";

    return true;
  }

  void analyzeCondition(Expr *Cond) {
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Cond)) {
      if (BO->getOpcode() == BO_LOr) {
        analyzeCondition(BO->getLHS());
        analyzeCondition(BO->getRHS());
        return;
      }
    }

    SourceRange CondRange = Cond->getSourceRange();
    std::string CondText = getSourceText(CondRange);

    const FileEntry *fileEntry = srcManager.getFileEntryForID(
        srcManager.getFileID(CondRange.getBegin()));
    unsigned int line = srcManager.getSpellingLineNumber(CondRange.getBegin());
    if (fileEntry)
      llvm::outs() << fileEntry->getName() << ":" << line << "\n";

    llvm::outs() << "Condition part: " << CondText << "\n";
  }

private:
  Rewriter &rewriter;
  SourceManager &srcManager;
  FunctionDecl *CurrentFunction;

  std::string getCurrentFunctionName() {
    return (CurrentFunction ? CurrentFunction->getNameInfo().getAsString()
                            : "N/A");
  }

  std::string getSourceText(SourceRange range) {
    return Lexer::getSourceText(CharSourceRange::getTokenRange(range),
                                srcManager, LangOptions(), 0)
        .str();
  }
};