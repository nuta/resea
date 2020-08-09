# Getting Started

## Prerequisites
- LLVM/Clang/LLD version 8.x or higher
- Python version 3.7 or higher
- QEMU
- Bochs *(optional)*
- GRUB and xorriso *(optional)*
- Docker *(optional: Linux ABI emulation uses Docker to build packages)*

### macOS
```
$ brew install llvm python3 qemu bochs i386-elf-grub xorriso mtools sparse
$ brew cask install gcc-arm-embedded
$ pip3 install -r tools/requirements.txt
```

### Ubuntu 20.04
```
$ apt install llvm-8 lld-8 clang-8 qemu bochs grub2 xorriso mtools \
    python3 python3-dev python3-pip python3-setuptools \
    gcc-arm-none-eabi
$ pip3 install --user -r tools/requirements.txt
$ cargo install cargo-xbuild
```

### Rust
If you'd like to work on Rust applications, install Rust toolchain:

1. Install [rustup](https://rustup.rs/).
2. Add `rust-src` component: `rustup component add rust-src --toolchain nightly`.

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
