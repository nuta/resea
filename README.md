Resea
=====
[![Build Status](https://travis-ci.com/seiyanuta/resea.svg?branch=master)](https://travis-ci.com/seiyanuta/resea)


![screenshot](https://seiya.me/resea/screenshot.png)

Resea *[ríːseə]* is a microkernel-based operating system written from scratch featuring:

- **A *pure* microkernel written in C:** provides only primitive such as
  process/thread/memory management and channel-based message passing.
- **Userland applications entirely written in Rust:** TCP/IP server,
  FAT32 file system driver, device drivers, and other userland applications are written in Rust.
- **Basic x86_64 support:** PS/2 keyboard, text-mode screen, e1000 network card, IDE (hard disk)
  are currently supported. Other CPU architecture support (e.g. ARM and RISC-V) are in the todo list yet.

Resea aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system, and fun to extend the functionality.

Read **[documentation](https://seiya.me/resea/docs)** to get started.

Documentation
-------------
**[Documentation](https://seiya.me/resea/docs)**

Contributing
------------
We accept bug reports, feature requests, and patches on [GitHub](https://github.com/nuta/resea).

License
-------
[CC0](https://creativecommons.org/publicdomain/zero/1.0/) or [MIT](https://opensource.org/licenses/MIT). Choose whichever you prefer.
