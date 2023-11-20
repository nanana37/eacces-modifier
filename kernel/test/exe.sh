#!/bin/bash
# trace-cmd

gcc my-cat.c -o a.out

sudo trace-cmd record -p function_graph --user hiron -F ./a.out

# Change log file name every time
timestamp=$(date +%Y-%m%d-%H%M)
logfile="log/log-"$timestamp".list"
trace-cmd report trace.dat > $logfile

echo $logfile "is created."
