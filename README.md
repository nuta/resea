Resea
=====
[![Build Status](https://travis-ci.com/seiyanuta/resea.svg?branch=master)](https://travis-ci.com/seiyanuta/resea)

Resea *[ríːseə]* is a pure microkernel-based operating system written from
scratch. It aims to be easy to learn the implementation and experience
microkernel-style development. Resea consists of a microkernel and servers.
The microkernel provides essential functionalities: process, thread, and
message passing (IPC). Servers are userland programs that provide services
such as memory allocation, device drivers, file system, and TCP/IP. See
**[Documentation](#documentation)** for details.

Road Map
--------
- [x] Kernel for x86_64
- [x] IPC benchmark app
- [ ] **`memmgr`:** the user-level memory management server **(WIP)**
- [ ] GUI server
- [ ] SMP support
- [ ] **`drvmgr`:** the device driver management server
- [ ] **`fsmgr`:** the file system management server
- [ ] virtio-blk device driver
- [ ] FAT file system server
- [ ] virtio-net device driver
- [ ] TCP/IP server

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
$ make defconfig            # Generate a default .config file.
$ make build                # Build a kernel executable (debug build).
$ make build BUILD=release  # Build a kernel executable (release build).
$ make build V=1            # Build a kernel executable with verbose command output.
$ make run                  # Run on QEMU (-nographic).
$ make run GUI=1            # Run on QEMU with graphical output.
$ make bochs                # Run on Bochs.
$ make clean                # Remove built files.
$ make docs                 # Generate a source code reference.
```

Documentation
-------------
- **[Design and Internals](https://github.com/seiyanuta/resea/blob/master/INTERNALS.md)**
- **[Developer's Guide](https://github.com/seiyanuta/resea/blob/master/HACKING.md)**

Contributing
------------
We receive bug reports, feature requests, and patches on [GitHub](https://github.com/seiyanuta/resea).

License
-------
CC0 or MIT. Choose whichever you prefer.
