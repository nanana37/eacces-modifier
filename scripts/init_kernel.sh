#!/bin/bash

CONFIG=localmodconfig

if [ $# -gt 0 ]; then
	CONFIG=$1
fi

make mrproper && make $CONFIG
