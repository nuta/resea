Resea
=====
[![Build Status](https://travis-ci.com/seiyanuta/resea.svg?branch=master)](https://travis-ci.com/seiyanuta/resea)

Resea *[ríːseə]* is a microkernel-based operating system written from scratch featuring:

- **A *pure* microkernel written in C:** provides only primitive services:
  process/thread/memory management and channel-based message passing.
- **Userland applications entirely written in Rust:** TCP/IP server,
  FAT32 file system driver, device drivers, and other userland applications are written in Rust.
- **Basic x86_64 support:** PS/2 keyboard, text-mode screen, e1000 network card, IDE (hard disk)
  are currently supported. Other CPU architecture support (such as ARM and RISC-V) are in the todo list yet.

Resea aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun to extend the functionality.

See **[Documentation](#documentation)** for details.

Prerequisites
-------------

**macOS:**
```
$ brew install llvm python3 qemu bochs i386-elf-grub xorriso mtools \
               doxygen graphviz
$ pip3 install -r tools/requirements.txt
```

**Ubuntu:**
```
$ sudo apt install llvm lld clang make git python3 python3-pip \
                   qemu bochs grub2 xorriso mtools \
                   doxygen graphviz
$ pip3 install -r tools/requirements.txt
```

Building
--------
```bash
$ make build         # Build a kernel executable.
$ make build V=1     # Build a kernel executable with verbose command output.

$ make run           # Run on QEMU with -nographic.
$ make run GUI=1     # Run on QEMU without -nographic.
$ make bochs         # Run on Bochs.

$ make docs          # Generate a source code reference.
$ make clean         # Remove built files.
```

Documentation
-------------
- **[Design](https://github.com/seiyanuta/resea/blob/master/docs/design.md)**
- **[Internals](https://github.com/seiyanuta/resea/blob/master/docs/internals.md)**
- **[Server Writer's Guide](https://github.com/seiyanuta/resea/blob/master/docs/server-writers-guide.md)**

Contributing
------------
We receive bug reports, feature requests, and patches on [GitHub](https://github.com/seiyanuta/resea).

License
-------
[CC0](https://creativecommons.org/publicdomain/zero/1.0/) or [MIT](https://opensource.org/licenses/MIT). Choose whichever you prefer.
