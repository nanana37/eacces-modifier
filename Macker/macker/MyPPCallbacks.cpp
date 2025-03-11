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
  StringRef FileName = CI.getSourceManager().getFilename(Range.getBegin());
  if (FileName.size() <= 0) {
    return;
  }
  unsigned int LineNumber =
      CI.getSourceManager().getSpellingLineNumber(Range.getBegin());

  errs() << "[MacroExpands]";
  errs() << "  " << FileName << ":" << LineNumber << "\n";
  errs() << MacroNameTok.getIdentifierInfo()->getName();

 // Get expanded text
 if (const MacroInfo *MI = MD.getMacroInfo()) {
    std::string ExpansionText;
    Preprocessor &PP = CI.getPreprocessor();
    
    // Get tokens from the macro definition
    for (const Token &Tok : MI->tokens()) {
      if (ExpansionText.size() > 0)
        ExpansionText += " ";
      ExpansionText += PP.getSpelling(Tok);
    }

    errs() << " => " << ExpansionText << "\n";
  }
}
