Resea
=====
[![Build Status](https://travis-ci.com/nuta/resea.svg?branch=master)](https://travis-ci.com/nuta/resea)

![screenshot](https://gist.githubusercontent.com/nuta/95388285b0d6efca0329641e8c52f4e7/raw/223000d67ba4a304698c48cd3eca8467dbc99b24/screenshot.png)

Resea *[ríːseə]* is a microkernel-based operating system written from scratch.
It aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun
to extend the functionality.

Features
--------
- A *pure* microkernel with x86_64 (SMP) support
- Simple storage server (kvs)
- TCP/IP server
- Some device drivers (PS/2 keyboard, e1000 network card, and text-mode screen)

Road Map in 2020
----------------
- Documentation
- GUI server
- Kernel-level handles like [IPC Gate in Fiasco (L4)](https://l4re.org/doc/group__l4__kernel__object__gate__api.html)
- RISC-V support
- [Rust](https://www.rust-lang.org/) in userland

Contributing
------------
We accept bug reports, feature requests, and patches on
[GitHub](https://github.com/nuta/resea).

License
-------
[CC0](https://creativecommons.org/publicdomain/zero/1.0/) or
[MIT](https://opensource.org/licenses/MIT). Choose whichever you prefer.
