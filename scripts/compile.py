import subprocess
import sys

target = "example"
source_target = f"test/{target}.c"

compile_command = f"clang -fpass-plugin=build/permod/PermodPass.so {source_target}"
print("Compile command:\n$ " + compile_command)
subprocess.run(compile_command, shell=True)

if len(sys.argv) > 1:
    run_command = f"./a.out"
    print("Run command:\n$ " + run_command)
    subprocess.run(run_command, shell=True)
