#!/bin/sh

# sudo sysctl fs.protected_fifos=1
mknod /tmp/err-fifo p
touch /tmp/err-reg
