# Getting Started

## Prerequisites
- LLVM/Clang/LLD version 8.x or higher
- Python version 3.7 or higher
- QEMU
- Bochs *(optional)*
- GRUB and xorriso *(optional)*

### macOS
```
$ brew install llvm python3 qemu bochs i386-elf-grub xorriso
```

### Ubuntu 18.04
```
$ apt install llvm-8 lld-8 clang-8 qemu bochs grub2 xorriso \
    python3 python3-dev python3-pip python3-setuptools
```

## Building Resea
```
$ make build               # Build a kernel executable.
$ make iso                 # Build an ISO image.
$ make build BUILD=release # Build a kernel executable (release build).
$ make build V=1           # Build with verbose command output.
$ make clean               # Remove generated files.
```

## Running Resea
```
$ make run GUI=1     # Run on QEMU.
$ make run           # Run on QEMU with -nographic.
$ make bochs         # Run on Bochs.
```
