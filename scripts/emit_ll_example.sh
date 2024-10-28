#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ~/eacces-modifier/build || exit
echo "Updating pass"
make

cd "$CWD" || exit
# argument
if [ $# -eq 0 ]; then
  echo "No arguments supplied"
  exit 1
fi

echo "Compiling $1"
clang -g -fno-discard-value-names -fpass-plugin="$HOME"/eacces-modifier/build/permod/PermodPass.so -emit-llvm -S "$1"
echo "Done"
