#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ${PERMOD_DIR}/build || exit
echo "Updating pass"
make

cd "$CWD" || exit
# argument
if [ $# -eq 0 ]; then
  echo "No arguments supplied"
  exit 1
fi

echo "Compiling rtlib"
clang -c ${PERMOD_DIR}/rtlib/rtlib.c

echo "Compiling $1"
clang -c -g -fno-discard-value-names -fpass-plugin="$PERMOD_DIR"/build/permod/PermodPass.so "$1".c

clang "$1".o rtlib.o

echo "Done"
