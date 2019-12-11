# Getting Started

## Setting up a Build Environment

### Install Packages (macOS)
```
$ brew install llvm python3 qemu bochs i386-elf-grub xorriso mtools 
```

### Install Packages (Ubuntu 18.04 or later)
```
$ sudo apt install llvm lld clang make git \
    python3 python3-pip python3-dev python3-setuptools \
    qemu bochs grub2 xorriso mtools
```

### Install Python Packages
```
$ pip3 install -r tools/requirements.txt
```

### Install Nightly Rust

1. Install [Rust toolchain using rustup](https://rustup.rs/).
2. Install the `rust-src` component:
```
$ rustup component add rust-src
```
3. Install xargo:
```
$ cargo install xargo
```

## Building Resea
```bash
$ make build               # Build a kernel executable.
$ make iso                 # Build an ISO image.
$ make build BUILD=release # Build a kernel executable (release build).
$ make build V=1           # Build with verbose command output.

$ make docs          # Generate docs.
$ make clean         # Remove built files.
```

## Running Resea
```bash
$ make run GUI=1     # Run on QEMU.
$ make run           # Run on QEMU with -nographic.
$ make bochs         # Run on Bochs.
```
