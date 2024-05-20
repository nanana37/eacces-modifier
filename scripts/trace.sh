#!/bin/sh

sudo trace-cmd record -p function_graph --user $1 -F $2 && trace-cmd report trace.dat >trace.list
