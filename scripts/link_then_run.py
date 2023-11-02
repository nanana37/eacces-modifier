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
clang_flag = f"-fpass-plugin=build/permod/PermodPass.{extension} -fno-discard-value-names"
compile_command = f"clang {clang_flag} -c {source_target} -o bin/{target}.o"
print("Compile command:\n$ " + compile_command)
subprocess.run(compile_command, shell=True)

# Link
link_command = f"cc bin/{target}.o bin/{rtlib}.o -o a.out"
print("Link command:\n$ " + link_command)
subprocess.run(link_command, shell=True)


if len(sys.argv) > 2 and sys.argv[2] == "run":
    run_command = f"./a.out"
    print("Run command:\n$ " + run_command)
    subprocess.run(run_command, shell=True)
