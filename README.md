# eacces-modifier

A pass to make "Permission denied" (errno: EACCES) detailed.

Used [Adrian Sampson's LLLVM turorial](https://github.com/sampsyo/llvm-pass-skeleton) as template.
([Blog](https://www.cs.cornell.edu/~asampson/blog/llvm.html))

## Build

### Environment (Prerequisites?)

- clang@17
  - The lower version may not work.
  - We are using [the New Pass Manager](https://llvm.org/docs/NewPassManager.html).

### Build command

This makes a shared object `build/permod/PermodPass.{so|dylib}` (`.so` on Linux / `.dylib` on macOS).

```
$ cd eacces-modifier
$ mkdir build
$ cd build
$ cmake ..  # Generate the Makefile.
$ make  # Actually build the pass.
```


## Usage

### Apply to the entire kernel

To apply the pass to the kernel, you need to clone [torvalds/linux](https://github.com/torvalds/linux) and prepare to build.
This [site](https://phoenixnap.com/kb/build-linux-kernel) explains well how to build.


You can change the `-j` option.
If `-j 1`, the compile-time log will be output sequentially.

```
# In the build dir of Linux kernel
$ make -j 8 \
    CC=clang \
    KCFLAGS="-fno-discard-value-names \
    -fpass-plugin=path_to_build/permod/PermodPass.so"
```

### Apply to a specific file

The pass can be applied to both a spcific file, a piece of Linux, and your original test file.


Simple example:

**A piece of Linux**

```
# In the build dir of Linux kernel
$ make CC=clang \
    KCFLAGS="-fno-discard-value-names \
    -fpass-plugin=path_to_build/permod/PermodPass.so" \
    fs/namei.c
```

**Your original file**

```
$ clang -fpass-plugin=path_to_build/permod/PermodPass.so something.c
```





