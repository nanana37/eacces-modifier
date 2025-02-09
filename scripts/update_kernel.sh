#!/bin/bash

# remember current dir
CWD=$(pwd)
PERMOD_DIR=$HOME/eacces-modifier
VERSION=$(date +%Y%m%d_%H%M)

# update pass
cd ~/eacces-modifier/build || exit
make

cd "$CWD" || exit
make \
  -j"$(nproc)" \
  CC=clang \
  LOCALVERSION=-mod-"$VERSION" \
  KCFLAGS="-g -fno-discard-value-names -fpass-plugin=$PERMOD_DIR/build/permod/PermodPass.so" \
  2>"$HOME"/filter/tmp/permod-"$VERSION"

# sudo make modules_install && sudo make install
