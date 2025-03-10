# Macker: Macro Tracker as Clang Plugin

## Quick Start
```sh
mkdir build && cd build
cmake .. && make
cd ..
clang -Xclang -load -Xclang ${MACKER_DIR}/build/macker/MyClangPlugin.so \
  -Xclang -plugin -Xclang my-ppcallback-plugin \
  test/test.c
```

### For Linux Kernel
```sh
rm fs/namei.o
yes "" | make CC=clang \
  KCFLAGS="-Xclang -load -Xclang ${MACKER_DIR}/build/macker/MyClangPlugin.so -Xclang -plugin -Xclang my-ppcallback-plugin" \
  fs/namei.o
```