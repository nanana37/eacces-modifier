#!/bin/bash

KERNEL_SRC="$1"
TARGET="$2"

if [ -z "$KERNEL_SRC" ] || [ -z "$TARGET" ]; then
  echo "Usage: $0 <path-to-kernel-source> <target.o>"
  echo "To build whole kernel: $0 <path-to-kernel-source> all"
  exit 1
fi

cmake --preset kernel
cmake --build --preset kernel

BUILD_DIR=/home/vscode/eacces-modifier/build-kernel
MACKER_REL_PATH=Macker/macker/Macker.so
PERMOD_REL_PATH=Permod/permod/PermodPass.so

# Build kernel with plugins
cd "$KERNEL_SRC"

# TODO: Build whole kernel with rtlib
# Need to put Permod/rtlib in the kernel source tree
if [ "$TARGET" == "all" ]; then
  yes "" | make CC=clang KCFLAGS="-g -fno-discard-value-names -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} -fpass-plugin=${BUILD_DIR}/${PERMOD_REL_PATH}"
  exit 0
fi

# Build target with plugins
rm $TARGET
yes "" | make CC=clang KCFLAGS="-g -fno-discard-value-names -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} -fpass-plugin=${BUILD_DIR}/${PERMOD_REL_PATH}" $TARGET
