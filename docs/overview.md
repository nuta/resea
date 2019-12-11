# Overview
Resea is a microkernel-based general purpose operating system written from
scratch. 

Resea aims to provide an attractive developer experience and be *hackable*:
intuitive to understand the whole design, easy to customize the system,
and fun to extend the functionality. To achieve the aims, we are **not** going
to implement an integrated POSIX support like MINIX3 since it would complicate
the system design.

## Features
- **A *pure* microkernel written in C:** provides only primitive such as
  process/thread/memory management and channel-based message passing.
- **Userland applications entirely written in Rust:** TCP/IP server,
  FAT32 file system driver, device drivers, and other userland applications are
  written in Rust.
- **Basic x86_64 support:** PS/2 keyboard, text-mode screen, e1000 network card,
  IDE (hard disk) are currently supported. Other CPU architecture support (e.g.
  ARM and RISC-V) are in the todo list yet.

## What Resea is for
- Building a (needlessly) well-decomposed operating system.
- Writing a bare-metal program for a research purpose.
- Learning microkernel concepts and its implementation.

## What Resea is NOT for
- A next-generation super cool alternative to Windows, macOS, and Linux.
- Yet another *NIX implementation based on microkernel design.
