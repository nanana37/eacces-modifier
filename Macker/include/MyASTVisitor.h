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
private:
  std::string escapeCSV(const std::string &str) {
    if (str.find('"') == std::string::npos && str.find(',') == std::string::npos) {
      return str;
    }
    std::string escaped = str;
    // Replace quotes with double quotes
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
      escaped.replace(pos, 1, "\"\"");
      pos += 2;
    }
    return "\"" + escaped + "\"";
  }
  
  std::string getFuncName(FunctionDecl *Func) {
    if (Func) {
      return Func->getNameInfo().getAsString();
    }
    return "unknown";
  }
  
  void getFileAndLine(SourceLocation Loc, std::string &File, int &Line) {
    if (!Loc.isValid()) {
      File = "unknown";
      Line = 0;
      return;
    }
  
    // Get both spelling and expansion locations
    SourceLocation SpellingLoc = srcManager.getSpellingLoc(Loc);
    SourceLocation ExpansionLoc = srcManager.getExpansionLoc(Loc);
  
    // If in a macro, use expansion location
    if (srcManager.isMacroArgExpansion(Loc) || srcManager.isMacroBodyExpansion(Loc)) {
      if (const FileEntry *FE = srcManager.getFileEntryForID(srcManager.getFileID(ExpansionLoc))) {
        File = FE->getName();
        Line = srcManager.getExpansionLineNumber(ExpansionLoc);
      } else {
        File = "unknown-macro";
        Line = 0;
      }
    } else {
      if (const FileEntry *FE = srcManager.getFileEntryForID(srcManager.getFileID(SpellingLoc))) {
        File = FE->getName();
        Line = srcManager.getSpellingLineNumber(SpellingLoc);
      } else {
        File = "unknown-file";
        Line = 0;
      }
    }
  }
  
  void writeCSVRow(const std::string &Function, const std::string &File, 
                  int Line, const std::string &StmtType, const std::string &Condition) {
    llvm::outs() << escapeCSV(Function) << ","
                << escapeCSV(File) << ","
                << Line << ","
                << escapeCSV(StmtType) << ","
                << escapeCSV(Condition) << "\n";
  }

public:
  explicit MyASTVisitor(Rewriter &R, SourceManager &SM)
      : rewriter(R), srcManager(SM), CurrentFunction(nullptr) {
    // Write CSV header
    llvm::outs() << "Function,File,Line,StatementType,Condition\n";
  }

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

    analyzeIfCondition(Cond);
    return true;
  }

  void analyzeIfCondition(Expr *Cond) {
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

  bool VisitSwitchStmt(SwitchStmt *Switch) {
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

  bool VisitCaseStmt(CaseStmt *Case) {
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

private:
  Rewriter &rewriter;
  SourceManager &srcManager;
  FunctionDecl *CurrentFunction;

  std::string getSourceText(SourceRange range) {
    return Lexer::getSourceText(CharSourceRange::getTokenRange(range),
                                srcManager, LangOptions(), 0)
        .str();
  }
};