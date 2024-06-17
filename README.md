# eacces-modifier

A pass to make "Permission denied" (errno: EACCES) detailed.

## What is this?

We can't identify the reason between Error1 and Error2 in the following code.

```c
if ( flag & MASK1 ) {
  return -EACCES;  // Error1
}
if ( flag & MASK2 ) {
  return -EACCES;  // Error2
}
```

Because they are both `EACCES`, and says "Permission denied".

```bash
$ ./a.out
Permision denied
```

This tool will make logs about:

- Error1 caused by `flag & MASK1`
- Error2 caused by `flag & MASK2`

## Build

### Environment (Prerequisites?)

- clang@17
  - The lower version may not work.
  - We are using [the New Pass Manager](https://llvm.org/docs/NewPassManager.html).

### Build command

This makes a shared object `build/permod/PermodPass.{so|dylib}` (`.so` on Linux / `.dylib` on macOS).

```bash
cd eacces-modifier
mkdir build
cd build
cmake ..  # Generate the Makefile.
make  # Actually build the pass.
```

See [Trouble shooting](#cmake-error) if you encounter a CMake error.

## Usage

### Apply to the entire kernel

To apply the pass to the kernel, you need to clone [torvalds/linux](https://github.com/torvalds/linux) and prepare to build.
This [site](https://phoenixnap.com/kb/build-linux-kernel) explains well how to build.

You can change the `-j` option.
If `-j 1`, the compile-time log will be output sequentially.

```bash
# In the build dir of Linux kernel
make -j 8 \
  CC=clang \
  KCFLAGS="-fno-discard-value-names \
  -fpass-plugin=path_to_build/permod/PermodPass.so"
```

The logs will be in `/var/log/kern.log`, because we use `printk`.

### Apply to a specific file

The pass can be applied to both a spcific file, a piece of Linux, and your original test file.

Simple example:

**A piece of Linux**

```bash
# In the build dir of Linux kernel
make CC=clang \
  KCFLAGS="-fno-discard-value-names \
  -fpass-plugin=path_to_build/permod/PermodPass.so" \
  fs/namei.c
```

**Your original file**

```bash
clang -fpass-plugin=path_to_build/permod/PermodPass.so something.c
```

### Using Script

1. Apply to the entire kernel

- `scripts/update_kernel.sh` to build & install the kernel.
- Use inside the kernel's build dir.

2. Apply to a piece of Linux

- `scripts/update_partial.sh <filename>`
- Use inside the kernel's build dir.

3. Apply to your original file

- `scripts/update_example.sh <filename>`

4. Emit IR for your original file

- `scripts/emit_ll_example.sh <filename>`

## Coccinelle

We use [Coccinelle](https://www.kernel.org/doc/html/v4.18/dev-tools/coccinelle.html) to replace ternary operator to if-else.

### Why?

Linux Kernel uses ternary operator to set return value.

For example:

```c
return acl ? -EACCES; 0;
```

This C code is translated to `select` instruction on LLVM IR.

```bash
bb:
  %tobool = cmp ne %acl, 0
  %retval = select %tobool, -13, 0
  br label %return

return:
  ret %retval
```

Permod overlooks the condition of the `select`, because `select` includes a branch inside itself.

### Usage

Create patch at linux repository.

```bash
make coccicheck MODE=patch COCCI=path_to_permod/scripts/coccinelle/sel_to_br.cocci > path_to_patch
```

Apply patch.

```bash
patch -p1 < path_to_patch
```

## Trouble shooting

### CMake error

When building with CMake, you may encounter the following error:

```bash
CMake Error at CMakeLists.txt:13 (include):
  include could not find load file:

    AddLLVM


CMake Error at permod/CMakeLists.txt:1 (add_llvm_pass_plugin):
  Unknown CMake command "add_llvm_pass_plugin".


-- Configuring incomplete, errors occurred!
See also "/home/hiron/eacces-modifier/bui/CMakeFiles/CMakeOutput.log".
```

This is because the LLVM is not installed globally.
You need to give CMake the path to the `share/llvm/cmake`.
Do this after cleaning the build dir.

e.g.,

```bash
LLVM_DIR=/usr/lib/llvm-17/lib/cmake/llvm cmake ..

# For Homebrew
LLVM_DIR='brew --prefix llvm@17'/lib/cmake/llvm cmake ..
```

### No output

When you use wrong version of clang, the pass may not work.
Use clang 17.

Solution:

- a. Rewrite script
- b. Replace symlink to clang

### LSP 'clangd' cannot find the header files

Clangd cannot follow third-party header files.
Set `CMAKE_EXPORT_COMPILE_COMMANDS` to 1 for CMake command.

e.g.,

```bash
CMAKE_EXPORT_COMPILE_COMMANDS=1 cmake ..
```

## References

Used [Adrian Sampson's LLLVM turorial](https://github.com/sampsyo/llvm-pass-skeleton) as template.
([Blog](https://www.cs.cornell.edu/~asampson/blog/llvm.html))
