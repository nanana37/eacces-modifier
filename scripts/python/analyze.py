import datetime
import multiprocessing
import os
import subprocess
import time
from pathlib import Path

import click

from Permod import utils
from Permod.constants import LINUX_ROOT, LOG_DIR, PASS_PATH, RTLIB_DIR


@click.group()
def commands():
    pass


@commands.command()
@click.option("--target", "-t", default=LINUX_ROOT)
# @click.option("--file", "-f", default=None)
# @click.option("--measure", "-m", is_flag=True)
def linux(target, file, measure):
    print(f"Start running analyzer:{PASS_PATH}")
    tmplog = os.path.join(LOG_DIR, "tmplog")
    current = datetime.datetime.now().strftime("%Y_%m_%d_%H:%M")
    log = os.path.join(LOG_DIR, f"{current}.log")
    with open(tmplog, "w+") as f:
        compiler_flags = [f"-fpass-plugin={PASS_PATH}"]
        make_flags = []

        if measure:
            compiler_flags += ["-mllvm", "-measure"]

        if file:
            target_file = os.path.join(target, file)
            if os.path.exists(target_file):
                subprocess.run(["rm", target_file])
            make_flags.append(file)

        command = utils.make_linux_build_command(
            target, multiprocessing.cpu_count(), compiler_flags, make_flags
        )

        # Start analysis
        start = time.time()
        result = subprocess.run(command, stderr=subprocess.PIPE)
        end = time.time()
        # Finish Analysis

        f.write(result.stderr.decode("utf-8"))

    logfiles = [tmplog] + utils.get_log_files(Path(target))
    log_output = ""

    measure_output = ""
    for logfile in logfiles:
        with open(logfile, "r") as f:
            for line in f:
                # TODO: Make better keywords to filter
                # if "ERROR" in line or "LOG" in line:
                if "::" in line:
                    print(line, end="")
    with open(log, "w+") as f:
        f.write(log_output)

    if measure:
        measure_log = os.path.join(LOG_DIR, f"{current}_time.log")
        with open(measure_log, "w+") as f:
            f.write(measure_output)
        print(f"Logged time measure to {measure_log}")

    print(f"Done running command (Runtime - {end - start} sec): {command}")
    print(f"Logged to file {log}")


def compile_rtlib():
    command = ["clang", "-c", f"{RTLIB_DIR}/rtlib.c"]
    result = subprocess.run(command, stderr=subprocess.PIPE)
    print(result.stderr.decode("utf-8"))
    if result.returncode != 0:
        raise Exception("Failed to compile rtlib")


@commands.command()
@click.argument("target", type=click.Path(exists=True))
def test(target):
    print(f"Running test on {target}")
    target_files = utils.get_files(Path(target))
    additional_flags = [f"-fpass-plugin={PASS_PATH}"]

    print(f"Found {len(target_files)} tests")

    try:
        compile_rtlib()
    except Exception as e:
        print(e)
        return

    for target in target_files:
        print(f"[Running] {target}\r", end="")
        compile_command = [
            "clang",
            "-c",
            target,
        ] + utils.compilation_flags(additional_flags)
        result = subprocess.run(compile_command, stderr=subprocess.PIPE)
        logs = utils.remove_redundant_log(result.stderr.decode("utf-8"))

        if logs:
            # Add newline so that we do not override the "Running" tag
            print("")
            print(logs.strip())

        # Link
        compile_command = [
            "clang",
            target.with_suffix(".o"),
            "rtlib.o",
            "-o",
            target.with_suffix(".out"),
        ]
        result = subprocess.run(compile_command, stderr=subprocess.PIPE)
        print(result.stderr.decode("utf-8"))

        # Remove build artifacts
        rm_command = ["rm", target.with_suffix(".o")]
        result = subprocess.run(rm_command, stderr=subprocess.PIPE)
        print(result.stderr.decode("utf-8"))


if __name__ == "__main__":
    commands()
