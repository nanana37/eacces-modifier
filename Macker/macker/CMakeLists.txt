add_library(MyClangPlugin SHARED
  main.cpp
  MyPPCallbacks.cpp
  MyASTConsumer.cpp
  MyFrontendAction.cpp
)

target_link_libraries(MyClangPlugin
  clangBasic
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

set_target_properties(MyClangPlugin PROPERTIES
  PREFIX ""
  )
