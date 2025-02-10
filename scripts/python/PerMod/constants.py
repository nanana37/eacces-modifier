import os

HOME_DIR = os.path.expanduser("~")

### CONSTANT VARIABLES ###
PERMOD_ROOT = os.environ.get("PERMOD_ROOT", "/PerMod")
LINUX_ROOT = os.environ.get("LINUX_ROOT", "/linux")
LOG_DIR = os.path.abspath("/tmp/log")
BUILD_DIR = os.path.join(PERMOD_ROOT, "build")
PASS_PATH = os.path.join(BUILD_DIR, "permod", "PermodPass.so")
