# Macker: Macro Tracker as Clang Plugin

## Quick Start
```sh
clang -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} test/test.c
```

### For Linux Kernel
```sh
rm fs/namei.o
yes "" | make CC=clang \
  KCFLAGS="-fplugin=${BUILD_DIR}/${MACKER_REL_PATH}" \
  fs/namei.o
```