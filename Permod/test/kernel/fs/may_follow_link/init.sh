#!/bin/sh

mkdir out
cd out
touch err-target
ln -s $PWD/err-target /tmp/err-symlnk
