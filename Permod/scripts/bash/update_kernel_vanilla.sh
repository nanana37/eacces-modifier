#!/bin/bash

# remember current dir
CWD=$(pwd)
VERSION=vanilla

cd "$CWD" || exit
make -j"$(nproc)" CC=clang LOCALVERSION=-$VERSION && sudo make modules_install && sudo make install
