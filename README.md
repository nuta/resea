Resea
=====
[![Build Status](https://github.com/nuta/resea/workflows/CI/badge.svg)](https://github.com/nuta/resea/actions?query=workflow%3ACI)
[![Gitter chat](https://badges.gitter.im/resea/community.png)](https://gitter.im/resea/community)

![screenshot](https://gist.githubusercontent.com/nuta/18bb9fb757bcb547f2432c4fc5197dcf/raw/6faa6cd38b2ad23cfdbfdabff5107f99aead12f9/demo.gif)

Resea *[ríːseə]* is a microkernel-based operating system written from scratch.
It aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun
to extend the functionality.

Features
--------
- A *pure* microkernel with x86_64 (SMP) and experimental ARMv6-M (micro:bit) support.
- FAT file system server.
- TCP/IP server.
- Text-based user interface.
- Device drivers for PS/2 keyboard, e1000 network card, etc.
- Linux ABI emulation (experimental).

Road Map in 2020
----------------
- Stabilize APIs.
- Improve stability and performance.
- Documentation
- RISC-V support
- [Rust](https://www.rust-lang.org/) in userland
- Developer tools improvements: userland debugger, fuzzer, unit testing, etc.
- GUI server

Build Instructions
------------------
### Mac
```
brew install llvm python qemu bochs i386-elf-grub xorris
pip install -r tools/requirements.txt
make menuconfig
make
```
### Ubuntu
```
apt install llvm clang lld python3 qemu-system-x86 bochs grub2 xorriso git make
pip install -r tools/requirements.txt
make menuconfig
make
```

How to Run This Project
-----------------------
```
make run
```

Contributing
------------
We accept bug reports, feature requests, and patches on
[GitHub](https://github.com/nuta/resea).

License
-------
[CC0](https://creativecommons.org/publicdomain/zero/1.0/) or
[MIT](https://opensource.org/licenses/MIT). Choose whichever you prefer.
