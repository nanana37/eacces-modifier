#!/bin/bash

cd build
echo "Building..."
make

cd ..

echo "Running..."
clang -fpass-plugin=build/permod/PermodPass.so -g -fno-discard-value-names test/example.c

echo "Done!"
