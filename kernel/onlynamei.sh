#!/bin/bash

# remember current directory
export CWD=`pwd`

# cd to compile pass
cd $HOME/eacces-modifier/build
make

# cd to compile kernel
cd $HOME/linux-permod
bash ./update_namei.sh

# cd back to the original directory
cd $CWD

# all done, reboot now
