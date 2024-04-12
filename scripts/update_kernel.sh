#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ~/eacces-modifier/build
make

cd $CWD
make -j 8 CC=clang KCFLAGS="-g -fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" && sudo make modules_install && sudo make install
