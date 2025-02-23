#!/bin/sh
# Usage: sh trace.sh [-u <user>] [-p <plugin>] <command>

# init
user=$USER
plugin='function_graph'
command=''

while getopts 'p:u:' opt; do
	case ${opt} in
	u) user=$OPTARG ;;
	p) plugin=$OPTARG ;;
	\?) echo "Usage: $0 [-p plugin] [-u user] command" && exit 1 ;;
	:) echo "Invalid option: $OPTARG requires an argument" 1>&2 && exit 1 ;;
	esac
done

shift $((OPTIND - 1))
command=$@

if [ -z "$command" ]; then
	echo "Usage: $0 [-p plugin] [-u user] command"
	exit 1
fi

# echo running
echo "Running: '$command' as $user with plugin $plugin"
sudo trace-cmd record -p $plugin --user $user -F $command && trace-cmd report trace.dat >trace.list
