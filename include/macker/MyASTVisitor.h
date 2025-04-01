#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  explicit MyASTVisitor(Rewriter &R, SourceManager &SM);

  bool VisitFunctionDecl(FunctionDecl *Func);
  bool VisitIfStmt(IfStmt *If);
  void analyzeIfCondition(Expr *Cond);
  bool VisitSwitchStmt(SwitchStmt *Switch);
  bool VisitCaseStmt(CaseStmt *Case);

private:
  std::string getFuncName(FunctionDecl *Func);
  void getFileAndLine(SourceLocation Loc, std::string &File, int &Line);
  void writeCSVRow(const std::string &Function, const std::string &File,
                   int Line, const std::string &StmtType,
                   const std::string &Condition);
  std::string getSourceText(SourceRange range);

  Rewriter &rewriter;
  SourceManager &srcManager;
  FunctionDecl *CurrentFunction;
};