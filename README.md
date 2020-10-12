Resea
=====
![Integrated Tests](https://github.com/nuta/resea/workflows/Integrated%20Tests/badge.svg)
[![Gitter chat](https://badges.gitter.im/resea/community.svg)](https://gitter.im/resea/community)

![screenshot](https://gist.githubusercontent.com/nuta/42b36c50df15142ac25c3a5420607f2a/raw/e6c05de775f4649f6ba29638fd3ed8f40ea2f74f/screenshot.png)

Resea *[ríːseə]* is a microkernel-based operating system written from scratch.
It aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun
to extend the functionality.

See **[Documentation](https://resea.org/docs)** for more detials.

Features
--------
- A **minimalistic and policy-free microkernel** based operating system written entirely from scratch in C (and less than 5000 LoC). *Everything is message passing!*
- Supports **x86_64** (with SMP), **64-bit ARM** (Raspberry Pi 3), and **ARMv6-M** (micro:bit).
- Includes userspace servers like **TCP/IP protocol stack** and **FAT file system driver**.
- Provides **easy-to-use APIs** and every components are written in **single-threaded event-driven** approarch. It makes really easy to understand how Resea works and debug your code.
- Some attractive experimental features like **Linux ABI emulation** and **Rust support**.

See **[Road Map](https://github.com/nuta/resea/projects/1)** for planned new features and improvements.

## Quickstart
### macOS
```
brew install llvm python qemu
pip3 install --user -r tools/requirements.txt
make menuconfig
make run
```

### Ubuntu
```
apt install llvm clang lld python3 qemu-system make
pip3 install --user -r tools/requirements.txt
make menuconfig
make run
```

Community
---------
If you have any questions, feel free to talk to us on [Gitter](https://gitter.im/resea/community).

Contributing
------------
We accept bug reports, feature requests, and patches on
[GitHub](https://github.com/nuta/resea).

License
-------
[CC0](https://creativecommons.org/publicdomain/zero/1.0/) or
[MIT](https://opensource.org/licenses/MIT). Choose whichever you prefer.
