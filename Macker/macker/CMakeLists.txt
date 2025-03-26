add_library(Macker SHARED
  main.cpp
  MyPPCallbacks.cpp
  MyASTVisitor.cpp
  LogManager.cpp
  # MyFrontendAction.cpp
)

target_include_directories(Macker
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Macker
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

set_target_properties(Macker PROPERTIES
  PREFIX ""
)