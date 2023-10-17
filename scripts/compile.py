import subprocess
import sys
import os

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

compile_command = f"clang -fpass-plugin=build/permod/PermodPass.so {source_target}"
print("Compile command:\n$ " + compile_command)
subprocess.run(compile_command, shell=True)

if len(sys.argv) > 2 and sys.argv[2] == "run":
    run_command = f"./a.out"
    print("Run command:\n$ " + run_command)
    subprocess.run(run_command, shell=True)
