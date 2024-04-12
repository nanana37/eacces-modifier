#!/bin/bash

# remember current dir
CWD=$(pwd)

# update pass
cd ~/eacces-modifier/build
make

cd $CWD
# argument
if [ $# -eq 0 ]; then
	echo "No arguments supplied"
	exit 1
fi
rm $1
make -j 1 CC=clang KCFLAGS="-g -fno-discard-value-names -fpass-plugin=$HOME/eacces-modifier/build/permod/PermodPass.so" $1
