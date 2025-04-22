#include "macker/MyASTVisitor.h"
#include "macker/LogManager.h"
#include "macker/LogParser.h"
#include "clang/Lex/Lexer.h"

#include "utils/debug.h"
#include <sstream>

using namespace clang;
using namespace macker;

// Constructor implementation
MyASTVisitor::MyASTVisitor(Rewriter &R, SourceManager &SM,
                           macker::LogParser &Parser,
                           const std::string &TargetFile)
    : rewriter(R), srcManager(SM), logParser(Parser), CurrentFunction(nullptr) {
  logParser.parse(TargetFile);
  const auto &ParsedLogs = logParser.getParsedLogs();
  for (const auto &LogEntry : ParsedLogs) {
    if (LogEntry.FileName == TargetFile) {
      FilteredLogs.push_back(LogEntry);
      DEBUG_PRINT2("Parsed log entry: "
                   << LogEntry.FileName << ", " << LogEntry.LineNumber << ", "
                   << LogEntry.FunctionName << ", " << LogEntry.EventType
                   << ", " << LogEntry.Content << ", " << LogEntry.ExtraInfo
                   << "\n");
    }
  }
}

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
                               const std::string &Condition,
                               const std::string &Extra = "") {
  // Call LogManager with the correct parameter order to match our fields:
  // File, Line, Function, EventType, Content, ExtraInfo
  LogManager::getInstance().addEntry(StmtType,  // EventType
                                     File,      // File
                                     Line,      // Line
                                     Function,  // Function
                                     Condition, // Content
                                     Extra);    // ExtraInfo
}

bool MyASTVisitor::VisitFunctionDecl(FunctionDecl *Func) {
  if (Func->hasBody()) {
    CurrentFunction = Func;
    std::string FuncSignature = Func->getReturnType().getAsString() + " " +
                                Func->getNameInfo().getAsString() + "(";
    for (unsigned i = 0; i < Func->getNumParams(); ++i) {
      ParmVarDecl *Param = Func->getParamDecl(i);
      if (i > 0)
        FuncSignature += ", ";
      FuncSignature +=
          Param->getType().getAsString() + " " + Param->getNameAsString();
    }
    FuncSignature += ")";
    CurrentFunctionSignature = FuncSignature;
    DEBUG_PRINT2("Function signature: " << FuncSignature << "\n");
  }
  DEBUG_PRINT2("Function: " << *Func << "\n");
  // get the function signature
  return true;
}

bool MyASTVisitor::VisitIfStmt(IfStmt *If) {
  Expr *Cond = If->getCond();
  if (!Cond)
    return true;

  analyzeIfCondition(Cond);
  return true;
}

#ifdef HOGE
bool MyASTVisitor::VisitIfStmt(IfStmt *ifStmt) {
  auto cond = ifStmt->getCond();
  llvm::outs() << "ifStmt: "
               << ifStmt->getSourceRange().getBegin().printToString(srcManager)
               << "\n";
  if (auto *binOp = dyn_cast<clang::BinaryOperator>(cond)) {
    llvm::outs() << "BinaryOperator: "
                 << binOp->getSourceRange().getBegin().printToString(srcManager)
                 << "\n";
    if (binOp->isComparisonOp()) {
      llvm::outs() << "Comparison operator: "
                   << binOp->getSourceRange().getBegin().printToString(
                          srcManager)
                   << "\n";
      auto *lhs = binOp->getLHS()->IgnoreParenCasts();
      auto *rhs = binOp->getRHS()->IgnoreParenCasts();
      if (auto *lhsDeclRef = dyn_cast<DeclRefExpr>(lhs)) {
        auto *var = lhsDeclRef->getDecl();
        llvm::outs() << "Comparing variable: " << var->getName() << "\n";

        // try to find assignment to this variable in parent block
        if (CurrentFunction && CurrentFunction->hasBody()) {
          Stmt *body = CurrentFunction->getBody();
          for (Stmt *stmt : body->children()) {
            // Look for BinaryOperator (assignment)
            if (auto *assignOp = dyn_cast<clang::BinaryOperator>(stmt)) {
              if (assignOp->getOpcode() == BO_Assign) {
                auto *lhsAssign = assignOp->getLHS()->IgnoreParenCasts();
                auto *rhsAssign = assignOp->getRHS()->IgnoreParenCasts();

                if (auto *lhsAssignRef = dyn_cast<DeclRefExpr>(lhsAssign)) {
                  if (lhsAssignRef->getDecl() == var) {
                    llvm::outs()
                        << "Assignment to " << var->getName() << ": "
                        << assignOp->getBeginLoc().printToString(srcManager)
                        << "\n";
                    if (auto *call = dyn_cast<CallExpr>(rhsAssign)) {
                      llvm::outs() << "This is a function call assignment.\n";
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return true;
}
#endif

void MyASTVisitor::analyzeIfCondition(Expr *Cond) {
  if (clang::BinaryOperator *BO = dyn_cast<clang::BinaryOperator>(Cond)) {
    if (BO->getOpcode() == BO_LOr || BO->getOpcode() == BO_LAnd) {
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

  std::string ExtraInfo;
  // Get all the source texts stored in the LineNumQueue
  for (const auto &LogEntry : FilteredLogs) {
    if (LogEntry.LineNumber != Line)
      continue;
    if (LogEntry.FileName != File)
      continue;
    // if (LogEntry.FunctionName != getFuncName(CurrentFunction))
    //   continue;
    // Found the log entry
    DEBUG_PRINT2("Content: " << LogEntry.Content << "\n");
    // parse the LogEntry.Content into LineNumQueue
    // e.g., "1,2,3,4" -> {1, 2, 3, 4}
    std::vector<unsigned> LineNumQueue;
    std::stringstream ss(LogEntry.Content);
    std::string token;
    while (std::getline(ss, token, ',')) {
      try {
        LineNumQueue.push_back(std::stoul(token));
      } catch (const std::exception &e) {
        DEBUG_PRINT("Error parsing token: " << token << ", " << e.what()
                                            << "\n");
      }
    }

    while (!LineNumQueue.empty()) {
      unsigned LineNum = LineNumQueue.back();
      LineNumQueue.pop_back();
      if (LineNum == Line) {
        continue;
      }
      std::string LineText = getSourceTextFromStartLine(LineNum);
      if (LineText.empty()) {
        DEBUG_PRINT("Error: Empty line text for line number: " << LineNum
                                                               << "\n");
        continue;
      }
      ExtraInfo += LineText + ",";
    }
    // Remove the last comma
    if (!ExtraInfo.empty()) {
      ExtraInfo.pop_back();
    }
    break;
  }
  DEBUG_PRINT2("ExtraInfo: " << ExtraInfo << "\n");

  if (!CurrentFunctionSignature.empty()) {
    std::string CurrentFunctionFile;
    int CurrentFunctionLine;
    getFileAndLine(CurrentFunction->getLocation(),
                   CurrentFunctionFile,
                   CurrentFunctionLine);
    LogManager::getInstance().addEntry("signature",
                                       CurrentFunctionFile,
                                       CurrentFunctionLine,
                                       getFuncName(CurrentFunction),
                                       CurrentFunctionSignature,
                                       "");
    CurrentFunctionSignature.clear();
  }

  writeCSVRow(
      getFuncName(CurrentFunction), File, Line, "if", CondText, ExtraInfo);
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

  if (!CurrentFunctionSignature.empty()) {
    std::string CurrentFunctionFile;
    int CurrentFunctionLine;
    getFileAndLine(CurrentFunction->getLocation(),
                   CurrentFunctionFile,
                   CurrentFunctionLine);
    LogManager::getInstance().addEntry("signature",
                                       CurrentFunctionFile,
                                       CurrentFunctionLine,
                                       getFuncName(CurrentFunction),
                                       CurrentFunctionSignature,
                                       "");
    CurrentFunctionSignature.clear();
  }

  writeCSVRow(getFuncName(CurrentFunction), File, Line, "switch", CondText);
  return true;
}

bool MyASTVisitor::VisitCaseStmt(CaseStmt *Case) {
  Expr *LHS = Case->getLHS();
  if (!LHS)
    return true;

  SourceRange CaseRange = LHS->getSourceRange();
  std::string CaseText = getSourceText(CaseRange);
  std::string ValueText = CaseText;

  std::string File;
  int Line;
  getFileAndLine(CaseRange.getBegin(), File, Line);

  // Find if the case text is being expanded as a macro
  auto EMList = LogManager::getInstance().getExpandedMacros();
  const auto &EM =
      std::find_if(EMList.begin(),
                   EMList.end(),
                   [&](const LogManager::ExpandedMacroInfo &EM) {
                     return EM.FileName == File && EM.LineNumber == Line;
                   });
  if (EM != EMList.end() && EM->MacroName == CaseText) {
    DEBUG_PRINT2("Found expanded macro: " << EM->MacroName << "\n");
    ValueText = EM->MacroValue;
  }
  writeCSVRow(
      getFuncName(CurrentFunction), File, Line, "case", CaseText, ValueText);
  return true;
}

std::string MyASTVisitor::getSourceText(SourceRange range) {
  std::string Text =
      Lexer::getSourceText(
          CharSourceRange::getTokenRange(range), srcManager, LangOptions(), 0)
          .str();
  Text.erase(std::remove(Text.begin(), Text.end(), '\n'), Text.end());
  return Text;
}

/* FIXME: Use clang library
 idea: prepare a map of line numbers to source text on
 RecursiveASTVisitor::VisitStmt()
 */
std::string MyASTVisitor::getSourceTextFromStartLine(int startLine) {
  // Get the file ID for the main field
  FileID fileID = srcManager.getMainFileID();

  // Get the start location of the specified line
  SourceLocation lineStart = srcManager.translateLineCol(fileID, startLine, 1);

  if (!lineStart.isValid()) {
    return "Invalid start line number";
  }

  // Initialize variables for dynamic line detection
  SourceLocation currentLineStart = lineStart;
  SourceLocation nextLineStart;
  std::string result;

  while (true) {
    // Get the start location of the next line
    nextLineStart = srcManager.translateLineCol(fileID, startLine + 1, 1);

    // If the next line is invalid, assume we've reached the end of the file
    if (!nextLineStart.isValid()) {
      nextLineStart = srcManager.getLocForEndOfFile(fileID);
    }

    // Create a source range for the current line
    SourceRange lineRange(currentLineStart, nextLineStart.getLocWithOffset(-1));

    // Extract the source text for the current line
    std::string lineText = getSourceText(lineRange);
    result += lineText;

    // Check if the current line ends with a semicolon or closing parenthesis
    if (!lineText.empty() &&
        (lineText.back() == ';' || lineText.back() == ')')) {
      break;
    }

    // Move to the next line
    currentLineStart = nextLineStart;
    startLine++;
  }

  return result;
}