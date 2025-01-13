#!/bin/bash

# remember current dir
CWD=$(pwd)
VERSION=mod-$(date +%Y%m%d)

# update pass
cd ~/eacces-modifier/build || exit
make

cd "$CWD" || exit
make -j"$(nproc)" CC=clang LOCALVERSION=-$VERSION KCFLAGS="-g -fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" 2>$HOME/filter/tmp/permod-$(date +%Y%m%d) && sudo make modules_install && sudo make install
