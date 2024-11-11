import sys
import re

def extract_function_name_and_flag():
    input_data = sys.stdin.read()

    # extract function name and flag
    function_name_pattern = r'\[Permod\] (.+?) returned'
    ext_flag_pattern = r'\(ext\)(0x[0-9a-fA-F]+)'
    dst_flag_pattern = r'\(dst\)(0x[0-9a-fA-F]+)'

    function_name_match = re.search(function_name_pattern, input_data)
    ext_flag_match = re.search(ext_flag_pattern, input_data)
    dst_flag_match = re.search(dst_flag_pattern, input_data)

    function_name = function_name_match.group(1).strip()
    ext_flag = ext_flag_match.group(1).strip()
    dst_flag = dst_flag_match.group(1).strip()

    return function_name, ext_flag, dst_flag

def filter_file_contents(filename, function_name, ext_flag):
    with open(filename, 'r') as file:
        lines = file.readlines()

    selected_lines = []
    for i, line in enumerate(lines):
        if line.startswith(function_name):
            # extract [num]
            # e.g. hoge::hoge()#0: a == b -> [num] = 0
            num_start = line.index('#') + 1
            num_end = line.index(': ')
            num = int(line[num_start:num_end])
            
            # compare with ext_flag
            if ext_flag & (1 << num):
                selected_lines.append(line.strip())

    return selected_lines


# Usage example
if __name__ == "__main__":
    # path to build log
    filename = 'tmp/permod-full-1107'

    # input from runtime log
    function_name = '/home/hiron/repo/github/torvalds/linux/fs/namei.c::inode_permission()'
    ext_flag = 0x5

    result = extract_function_name_and_flag()
    function_name = result[0]
    ext_flag = int(result[1], 16)

    result = filter_file_contents(filename, function_name, ext_flag)

    for line in result:
        print(line)

