# eacces-modifier

This repository contains the code for the EACCES Modifier project. The project aims to modify the code (mainly of the Linux kernel) to detect and handle EACCES errors.

## Contents

- [Macker](Macker/)
  - Clang Plugin to get macro expansion information.
- [Permod](Permod/)
  - LLVM Pass to instrument the code to detect EACCES errors.