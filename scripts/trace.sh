#!/bin/sh
# Usage: sh trace.sh <user> <command>

user=$1
shift
command=$@

echo "user=$user"
echo "command=$command"

sudo trace-cmd record -p function_graph --user $user -F $command && trace-cmd report trace.dat >trace.list
