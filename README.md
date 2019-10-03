Resea
=====
[![Build Status](https://travis-ci.com/seiyanuta/resea.svg?branch=master)](https://travis-ci.com/seiyanuta/resea)

Resea *[ríːseə]* is an operating system written from scratch based on a *pure*
microkernel: the kernel provides only primitives such as process/thread
management and channel-based IPC (message passing). Major features like physical
memory manager, file system, device drivers are implemented as isolated
userspace programs.

Resea aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun
to extend the functionality.

See **[Documentation](#documentation)** for details.

Road Map
--------
- [x] Kernel for x86_64
- [x] IPC benchmark app
- [x] The user-level memory management server
- [ ] virtio-blk device driver **(WIP)**
- [ ] FAT file system server
- [ ] GUI server **(WIP)**
- [ ] virtio-net device driver
- [ ] TCP/IP server
- [ ] Rust support in userspace
- [ ] SMP support
- [ ] ARM support
- [ ] Port Doom
- [ ] Write documentation

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
$ make menuconfig    # Edit build configuration.

$ make build         # Build a kernel executable.
$ make build V=1     # Build a kernel executable with verbose command output.

$ make run           # Run on QEMU.
$ make run NOGUI=1   # Run on QEMU with -nographic.
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
