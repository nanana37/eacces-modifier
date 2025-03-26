# Macker: Macro Tracker as Clang Plugin

## Quick Start
```sh
mkdir build && cd build
cmake .. && make
cd ..
clang -fplugin=${MACKER_DIR}/build/macker/Macker.so test/test.c
```

### For Linux Kernel
```sh
rm fs/namei.o
yes "" | make CC=clang \
  KCFLAGS="-fplugin=${MACKER_DIR}/build/macker/Macker.so" \
  fs/namei.o
```