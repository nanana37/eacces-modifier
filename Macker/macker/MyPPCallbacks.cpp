#include "macker/MyPPCallbacks.h"
#include "macker/LogManager.h"

using namespace clang;
using namespace llvm;
using namespace macker;

MyPPCallbacks::MyPPCallbacks(CompilerInstance &CI) : CI(CI) {
  // No header needed - LogManager handles this
}

void MyPPCallbacks::MacroDefined(const Token &MacroNameTok,
                                 const MacroDirective *MD) {
#if defined(MACRO_DEF_TRACKING)
  const IdentifierInfo *II = MacroNameTok.getIdentifierInfo();
  if (!II)
    return;

  const MacroInfo *MI = MD->getMacroInfo();
  if (!MI)
    return;

  StringRef MacroName = II->getName();

  std::string MacroValue = getTokenString(MI);

  // Get file and line info
  SourceLocation Loc = MacroNameTok.getLocation();
  std::string FileName;
  unsigned int LineNumber;

  getFileAndLine(Loc, FileName, LineNumber);

  // Use LogManager for consistent output
  LogManager::getInstance().addEntry("MacroDefined",
                                     FileName,
                                     LineNumber,
                                     "", // No function name for macros
                                     MacroName.str(),
                                     MacroValue);
#endif
}

void MyPPCallbacks::MacroExpands(const Token &MacroNameTok,
                                 const MacroDefinition &MD, SourceRange Range,
                                 const MacroArgs *Args) {
  StringRef FileName = CI.getSourceManager().getFilename(Range.getBegin());
  if (FileName.size() <= 0) {
    return;
  }

  unsigned int LineNumber =
      CI.getSourceManager().getSpellingLineNumber(Range.getBegin());

  StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();
  std::string ExpansionText;

  // Get expanded text
  if (const MacroInfo *MI = MD.getMacroInfo()) {
    ExpansionText = getTokenString(MI);
  }
  LogManager::getInstance().addExpandedMacro(
      MacroName.str(), ExpansionText, FileName.str(), LineNumber);

#if defined(MACRO_EXP_TRACKING)
  // Use LogManager for consistent output
  LogManager::getInstance().addEntry("MacroExpands",
                                     FileName.str(),
                                     LineNumber,
                                     "", // No function name for macros
                                     MacroName.str(),
                                     ExpansionText);
#endif
}

void MyPPCallbacks::getFileAndLine(SourceLocation Loc, std::string &FileName,
                                   unsigned int &LineNumber) {
  if (!Loc.isValid()) {
    FileName = "unknown";
    LineNumber = 0;
    return;
  }

  FileName = CI.getSourceManager().getFilename(Loc).str();
  LineNumber = CI.getSourceManager().getSpellingLineNumber(Loc);
  // LineNumber = CI.getSourceManager().getPresumedLineNumber(Loc);
}

std::string MyPPCallbacks::getTokenString(const MacroInfo *MI) {
  std::string Result;
  Preprocessor &PP = CI.getPreprocessor();

  for (const Token &Tok : MI->tokens()) {
    if (!Result.empty())
      Result += " ";
    Result += PP.getSpelling(Tok);
  }
  return Result;
}
