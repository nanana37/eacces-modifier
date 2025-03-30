#include "macker/MyASTVisitor.h"
#include "macker/LogManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace macker;

// Constructor implementation
MyASTVisitor::MyASTVisitor(Rewriter &R, SourceManager &SM)
    : rewriter(R), srcManager(SM), CurrentFunction(nullptr) {}

// Method implementations
std::string MyASTVisitor::getFuncName(FunctionDecl *Func) {
  if (Func) {
    return Func->getNameInfo().getAsString();
  }
  return "unknown";
}

void MyASTVisitor::getFileAndLine(SourceLocation Loc, std::string &File,
                                  int &Line) {
  if (!Loc.isValid()) {
    File = "unknown";
    Line = 0;
    return;
  }

  // Get both spelling and expansion locations
  SourceLocation SpellingLoc = srcManager.getSpellingLoc(Loc);
  SourceLocation ExpansionLoc = srcManager.getExpansionLoc(Loc);

  // If in a macro, use expansion location
  if (srcManager.isMacroArgExpansion(Loc) ||
      srcManager.isMacroBodyExpansion(Loc)) {
    if (const FileEntry *FE =
            srcManager.getFileEntryForID(srcManager.getFileID(ExpansionLoc))) {
      File = FE->getName();
      Line = srcManager.getExpansionLineNumber(ExpansionLoc);
    } else {
      File = "unknown-macro";
      Line = 0;
    }
  } else {
    if (const FileEntry *FE =
            srcManager.getFileEntryForID(srcManager.getFileID(SpellingLoc))) {
      File = FE->getName();
      Line = srcManager.getSpellingLineNumber(SpellingLoc);
    } else {
      File = "unknown-file";
      Line = 0;
    }
  }
}

void MyASTVisitor::writeCSVRow(const std::string &Function,
                               const std::string &File, int Line,
                               const std::string &StmtType,
                               const std::string &Condition) {
  // Call LogManager with the correct parameter order to match our fields:
  // File, Line, Function, EventType, Content, ExtraInfo
  LogManager::getInstance().addEntry(StmtType,  // EventType
                                     File,      // File
                                     Line,      // Line
                                     Function,  // Function
                                     Condition, // Content
                                     "");       // ExtraInfo
}

bool MyASTVisitor::VisitFunctionDecl(FunctionDecl *Func) {
  if (Func->hasBody()) {
    CurrentFunction = Func;
  }
  return true;
}

bool MyASTVisitor::VisitIfStmt(IfStmt *If) {
  Expr *Cond = If->getCond();
  if (!Cond)
    return true;

  analyzeIfCondition(Cond);
  return true;
}

void MyASTVisitor::analyzeIfCondition(Expr *Cond) {
  if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Cond)) {
    if (BO->getOpcode() == BO_LOr) {
      analyzeIfCondition(BO->getLHS());
      analyzeIfCondition(BO->getRHS());
      return;
    }
  }

  SourceRange CondRange = Cond->getSourceRange();
  std::string CondText = getSourceText(CondRange);

  std::string File;
  int Line;
  getFileAndLine(CondRange.getBegin(), File, Line);

  writeCSVRow(getFuncName(CurrentFunction), File, Line, "if", CondText);
}

bool MyASTVisitor::VisitSwitchStmt(SwitchStmt *Switch) {
  Expr *Cond = Switch->getCond();
  if (!Cond)
    return true;

  SourceRange CondRange = Cond->getSourceRange();
  std::string CondText = getSourceText(CondRange);

  std::string File;
  int Line;
  getFileAndLine(CondRange.getBegin(), File, Line);

  writeCSVRow(getFuncName(CurrentFunction), File, Line, "switch", CondText);
  return true;
}

bool MyASTVisitor::VisitCaseStmt(CaseStmt *Case) {
  Expr *LHS = Case->getLHS();
  if (!LHS)
    return true;

  SourceRange CaseRange = LHS->getSourceRange();
  std::string CaseText = getSourceText(CaseRange);

  std::string File;
  int Line;
  getFileAndLine(CaseRange.getBegin(), File, Line);

  writeCSVRow(getFuncName(CurrentFunction), File, Line, "case", CaseText);
  return true;
}

std::string MyASTVisitor::getSourceText(SourceRange range) {
  return Lexer::getSourceText(CharSourceRange::getTokenRange(range),
                              srcManager,
                              LangOptions(),
                              0)
      .str();
}