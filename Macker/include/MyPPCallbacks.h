#ifndef MY_PP_CALLBACKS_H
#define MY_PP_CALLBACKS_H

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang;

class MyPPCallbacks : public PPCallbacks {
public:
  explicit MyPPCallbacks(CompilerInstance &CI);
  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override;
  void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                    SourceRange Range, const MacroArgs *Args) override;

private:
  CompilerInstance &CI;
};

#endif // MY_PP_CALLBACKS_H
