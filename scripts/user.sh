#!/bin/bash

TARGET="$1"

if [ -z "$TARGET" ]; then
  echo "Usage: $0 <target.c>"
  exit 1
fi

cmake --preset default
cmake --build --preset default

BUILD_DIR=/home/vscode/eacces-modifier/build
MACKER_REL_PATH=Macker/macker/Macker.so
PERMOD_REL_PATH=Permod/permod/PermodPass.so
RTLIB_REL_PATH=Permod/rtlib/libPermod_rt.a

# Extract directory and filename from TARGET
TARGET_DIR=$(dirname "$TARGET")
TARGET_FILE=$(basename "$TARGET" .c)

# Change to target's directory
cd "$TARGET_DIR"

# Build target with plugins
clang -c -g -fno-discard-value-names -fplugin=${BUILD_DIR}/${MACKER_REL_PATH} -fpass-plugin=${BUILD_DIR}/${PERMOD_REL_PATH} "$TARGET_FILE".c

# Compile with rtlib
clang "$TARGET_FILE".o "$BUILD_DIR"/"$RTLIB_REL_PATH" -o "$TARGET_FILE".out