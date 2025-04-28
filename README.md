# eacces-modifier

This repository contains the code for the EACCES Modifier project. The project aims to modify the code (mainly of the Linux kernel) to detect and handle EACCES errors.

## Contents

- [Macker](Macker/)
  - Clang Plugin to get macro expansion information.
- [Permod](Permod/)
  - LLVM Pass to instrument the code to detect EACCES errors.

## Quick Start (Dev Container)

Use CMake on the root.

- Run for a user file

```sh
./scripts/user.sh <path-to-file>

# Example
./scripts/user.sh test/test.c # the result will be test/*.csv
```

- Run for a kernel file

```sh
./scripts/kernel.sh <path-to-kernel-src> <target.o>

# Example
./scripts/kernel.sh ../linux-kernel fs/namei.o
```

- Run for whole kernel

```sh
./scripts/kernel.sh <path-to-kernel-src> all

# Example
./scripts/kernel.sh ../linux-kernel all
```