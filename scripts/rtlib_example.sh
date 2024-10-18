#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ~/eacces-modifier/build
echo "Updating pass"
make

cd $CWD
# argument
if [ $# -eq 0 ]; then
  echo "No arguments supplied"
  exit 1
fi

echo "Compiling rtlib"
clang -c $HOME/eacces-modifier/rtlib/rtlib.c

echo "Compiling $1"
clang -c -g -fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so $1.c

clang $1.o rtlib.o

echo "Done"
