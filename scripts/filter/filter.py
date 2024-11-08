def filter_file_contents(filename, function_name, flag):
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
            
            # compare with flag
            if flag & (1 << num):
                selected_lines.append(line.strip())

    return selected_lines

# Usage example
if __name__ == "__main__":
    # path to build log
    filename = 'tmp/permod-full-1107'

    # input from runtime log
    function_name = '/home/hiron/repo/github/torvalds/linux/fs/namei.c::inode_permission()'
    flag = 0x5

    result = filter_file_contents(filename, function_name, flag)

    for line in result:
        print(line)

