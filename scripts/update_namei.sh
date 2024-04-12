#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ~/eacces-modifier/build
make

cd $CWD
rm fs/namei.o
make -j 1 CC=clang KCFLAGS="-fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" fs/namei.o

