# eacces-modifier

This repository contains the code for the EACCES Modifier project. The project aims to modify the code (mainly of the Linux kernel) to detect and handle EACCES errors.

## Contents

- [Macker](Macker/)
  - Clang Plugin to get macro expansion information.
- [Permod](Permod/)
  - LLVM Pass to instrument the code to detect EACCES errors.

## Quick Start (Dev Container)

Use CMake on the root.

### Run for a user file

```sh
cmake --preset user
cmake --build --preset user
```

```sh
clang -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} -fplugin=${BUILD_DIR}/${PERMOD_REL_PATH} test.c
```

### Run for a kernel file

```sh
cmake --preset default
cmake --build --preset default
```

In the kernel source directory:

```sh
rm fs/namei.o
yes "" | make CC=clang KCFLAGS="-g -fno-discard-value-names -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} -fplugin=${BUILD_DIR}/${PERMOD_REL_PATH}" fs/namei.o
```