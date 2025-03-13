add_library(Macker SHARED
  main.cpp
  MyPPCallbacks.cpp
  MyASTVisitor.cpp
  Utilities.cpp
  # MyFrontendAction.cpp
)

target_link_libraries(Macker clangBasic
  clangFrontend
  clangAST
  clangASTMatchers
  clangLex
  clangParse
  clangSema
  clangEdit
  clangDriver
  clangRewrite
  clangSerialization
  clangTooling
  clangAnalysis
  )

set_target_properties(Macker PROPERTIES
  PREFIX ""
  )
