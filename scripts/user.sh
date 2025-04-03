#!/bin/bash

# Parse command line arguments
while getopts ":mh" opt; do
  case $opt in
    m)
      echo "** Macro tracking enabled **"
      MODE_MACRO_TRACKING=1
      ;;
    h)
      echo "Usage: $0 [-m] <target.c>"
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

shift $((OPTIND - 1))
TARGET="$1"

if [ -z "$TARGET" ]; then
  echo "Usage: $0 [-m] <target.c>"
  exit 1
fi

cmake --preset default
cmake --build --preset default

BUILD_DIR=/home/vscode/eacces-modifier/build
MACKER_REL_PATH=Macker/macker/Macker.so
PERMOD_REL_PATH=Permod/permod/PermodPass.so
RTLIB_REL_PATH=Permod/rtlib/libPermod_rt.a

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

# Extract directory and filename from TARGET
TARGET_DIR=$(dirname "$TARGET")
TARGET_FILE=$(basename "$TARGET" .c)

# Change to target's directory
cd "$TARGET_DIR"

# Build target with plugins
clang -c $CLANG_OPTS "$TARGET_FILE".c

# Compile with rtlib
clang "$TARGET_FILE".o "$BUILD_DIR"/"$RTLIB_REL_PATH" -o "$TARGET_FILE".out