# Macker: Macro Tracker as Clang Plugin

## Quick Start
```sh
mkdir build && cd build
cmake -DLLVM_DIR=$(brew --prefix llvm@17)/lib/cmake/llvm \
      -DClang_DIR=$(brew --prefix llvm@17)/lib/cmake/clang \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
cd ..
clang -fsyntax-only \
  -Xclang -load -Xclang ./build/macker/MyClangPlugin.so \
  -Xclang -plugin -Xclang my-ppcallback-plugin \
  test/test.c
```
