#!/bin/bash

# Parse command line arguments
while getopts ":mh" opt; do
  case $opt in
    m)
      echo "** Macro tracking enabled **"
      MODE_MACRO_TRACKING=1
      ;;
    h)
      echo "Usage: $0 [-m] <path-to-kernel-source> <target.o>"
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

shift $((OPTIND - 1))
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

# Set up compilation flags
CLANG_OPTS="-g -fno-discard-value-names"
if [ -f "$BUILD_DIR/$MACKER_REL_PATH" ]; then
  CLANG_OPTS="$CLANG_OPTS -fplugin=$BUILD_DIR/$MACKER_REL_PATH"
  if [ $MODE_MACRO_TRACKING ]; then
    CLANG_OPTS="$CLANG_OPTS -fplugin-arg-macro_tracker-enable-pp"
  fi
else
  echo "Macker plugin not found at $BUILD_DIR/$MACKER_REL_PATH"
  exit 1
fi
if [ -f "$BUILD_DIR/$PERMOD_REL_PATH" ]; then
  CLANG_OPTS="$CLANG_OPTS -fpass-plugin=$BUILD_DIR/$PERMOD_REL_PATH"
else
  echo "Permod plugin not found at $BUILD_DIR/$PERMOD_REL_PATH"
  exit 1
fi

# Build kernel with plugins
cd "$KERNEL_SRC"

# TODO: Build whole kernel with rtlib
# Need to put Permod/rtlib in the kernel source tree
if [ "$TARGET" == "all" ]; then
  yes "" | make CC=clang KCFLAGS="$CLANG_OPTS"
  exit 0
fi

# Build target with plugins
rm $TARGET
yes "" | make CC=clang KCFLAGS="$CLANG_OPTS" $TARGET
