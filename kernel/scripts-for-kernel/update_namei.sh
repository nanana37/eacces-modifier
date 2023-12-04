#!/bin/bash

make -j 8 \
    CC=clang \
    KCFLAGS=" -fno-discard-value-names \
    -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" \
    fs/namei.o \
&& sudo make modules_install \
&& sudo make install
