#include "MyPPCallbacks.h"

using namespace llvm;

MyPPCallbacks::MyPPCallbacks(CompilerInstance &CI) : CI(CI) {}

void MyPPCallbacks::MacroDefined(const Token &MacroNameTok,
                                 const MacroDirective *MD) {
  const IdentifierInfo *II = MacroNameTok.getIdentifierInfo();
  if (!II)
    return;

  const MacroInfo *MI = MD->getMacroInfo();
  if (!MI)
    return;

  StringRef MacroName = II->getName();

  std::string MacroValue;
  Preprocessor &PP = CI.getPreprocessor();
  for (const Token &Tok : MI->tokens()) {
    StringRef TokSpelling = PP.getSpelling(Tok);
    MacroValue += TokSpelling.str() + " ";
  }

  errs() << "[MacroDefined] " << MacroName << " = " << MacroValue << "\n";
}

void MyPPCallbacks::MacroExpands(const Token &MacroNameTok,
                                 const MacroDefinition &MD, SourceRange Range,
                                 const MacroArgs *Args) {
  errs() << "[MacroExpands] " << MacroNameTok.getIdentifierInfo()->getName()
         << "\n";

  StringRef FileName = CI.getSourceManager().getFilename(Range.getBegin());
  unsigned int LineNumber =
      CI.getSourceManager().getSpellingLineNumber(Range.getBegin());

  // Print filename
  errs() << "  File: " << CI.getSourceManager().getFilename(Range.getBegin())
         << "\n";
  // Get line number
  errs() << "  Line: "
         << CI.getSourceManager().getSpellingLineNumber(Range.getBegin())
         << "\n";
}
