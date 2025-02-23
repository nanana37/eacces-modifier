import os
from dotenv import load_dotenv

# Prepare ".env" file
# e.g.,
# PERMOD_ROOT=/hoge/permod
# LINUX_ROOT=/hoge/linux
# LOG_DIR=/hoge/log
load_dotenv()

### CONSTANT VARIABLES ###
PERMOD_ROOT = os.environ.get("PERMOD_ROOT", "/eacces-modifier")
LINUX_ROOT = os.environ.get("LINUX_ROOT", "/linux")
LOG_DIR = os.environ.get("LOG_DIR", "/tmp/log")
BUILD_DIR = os.path.join(PERMOD_ROOT, "build")
PASS_PATH = os.path.join(BUILD_DIR, "permod", "PermodPass.so")
RTLIB_DIR = os.path.join(PERMOD_ROOT, "rtlib")
