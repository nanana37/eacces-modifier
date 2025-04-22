#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "macker/LogParser.h"
#include "permod/LogManager.h"

using namespace clang;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  explicit MyASTVisitor(Rewriter &R, SourceManager &SM,
                        macker::LogParser &Parser,
                        const std::string &TargetFile);

  bool VisitFunctionDecl(FunctionDecl *Func);
  bool VisitIfStmt(IfStmt *If);
  void analyzeIfCondition(Expr *Cond);
  bool VisitSwitchStmt(SwitchStmt *Switch);
  bool VisitCaseStmt(CaseStmt *Case);

private:
  std::string getFuncName(FunctionDecl *Func);
  void getFileAndLine(SourceLocation Loc, std::string &File, int &Line);
  void writeCSVRow(const std::string &Function, const std::string &File,
                   unsigned Line, const std::string &StmtType, int CondID,
                   const std::string &Condition, const std::string &Extra);
  std::string getSourceText(SourceRange range);
  std::string getSourceTextFromStartLine(int startLine);

  Rewriter &rewriter;
  SourceManager &srcManager;
  FunctionDecl *CurrentFunction;
  macker::LogParser &logParser;
  std::vector<permod::LogManager::LogEntry> FilteredLogs;
  std::string CurrentFunctionSignature;
};