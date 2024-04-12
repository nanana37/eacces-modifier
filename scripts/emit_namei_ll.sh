#!/bin/bash

CWD=$(pwd)
cd $HOME/eacces-modifier/build
make
cd $CWD

rm fs/namei.ll

make -j 1 CC=clang KCFLAGS="-g -fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" fs/namei.ll

