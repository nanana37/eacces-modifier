import subprocess
import sys
import os
from sys import platform

# Build
build_command = "make"
print("Build command:\n$ " + build_command)
# CD to the build directory
os.chdir("build")
subprocess.run(build_command, shell=True)
# CD back to the root directory
os.chdir("..")

# Get target
target = "example"
if len(sys.argv) > 1:
    target = sys.argv[1]
source_target = f"test/{target}.c"

# Prepare runtime library
rtlib = "rtlib"
source_rtlib = f"rtlib/{rtlib}.c"
compile_rtlib_command = f"cc -c {source_rtlib} -o bin/{rtlib}.o"
print("Compile runtime library command:\n$ " + compile_rtlib_command)
subprocess.run(compile_rtlib_command, shell=True)

# Compile
if platform == "darwin":
    extension = "dylib"
else:
    extension = "so"
# clang_flag = f"-fpass-plugin=build/permod/PermodPass.{extension} -fno-discard-value-names"
# compile_command = f"clang {clang_flag} -c {source_target} -o bin/{target}.o"
# print("Compile command:\n$ " + compile_command)
# subprocess.run(compile_command, shell=True)

# Apply to linux kernel
## Cd to the kernel directory
current_dir = os.getcwd()
home_dir = os.path.expanduser("~")
kernel_dir=f"{home_dir}/linux-mod"
os.chdir(kernel_dir)

## Remove the old object file
# rm_command = f"rm fs/namei.o"
# print("Remove command:\n$ " + rm_command)
# subprocess.run(rm_command, shell=True)

eacces_dir = f"{home_dir}/eacces-modifier" 
# target = f"fs/namei.o {eacces_dir}/bin/rtlib.o"
# target = f"fs/namei.o"
# target = f"{eacces_dir}/bin/rtlib.o"
target = ""
make_flag = f"-fno-discard-value-names -fpass-plugin={eacces_dir}/build/permod/PermodPass.{extension}"
apply_command = f"make -j 8 CC=clang KCFLAGS=\"{make_flag}\" {target}"
print("Apply command:\n$ " + apply_command)
subprocess.run(apply_command, shell=True)
os.chdir(current_dir)

# # Link
# link_command = f"cc bin/{target}.o bin/{rtlib}.o -o a.out"
# print("Link command:\n$ " + link_command)
# subprocess.run(link_command, shell=True)

