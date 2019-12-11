# Debugging
Resea Kernel is written in C. While some people say *"C is a bad language! Rewrite
everything in Rust!"*, C is a pretty good chioce for writing kernel because it
makes easy to understand what happens.

That said, debugging C code (especially in the kernel world) is really painful.
In this page, we'll walk you through some useful features for kernel debugging.

## printk
Use the following macros:

- `TRACE(fmt, ...)`
  - A trace message. Disabled on release build. 
- `DEBUG(fmt, ...)`
  - A debug message. Disabled on release build. 
- `INFO(fmt, ...)`
  - A info message.
- `WARN(fmt, ...)`
  - A warning message.
- `OOPS(fmt, ...)`
  - Same as `WARN` but it also prints a backtrace.
- `OOPS(expr)`
  - Prints an oops message if `expr != OK`.
- `PANIC(fmt, ...)`
  - Kernel panic. It prints the message and halts the CPU.
- `BUG(fmt, ...)`
  - An unexpected situation occurred in the kernel (a bug). It prints the message and halts the CPU.

## Backtrace
- `backtrace()`
  - Prints a backtrace like the following output. We recommend to use `OOPS` macro instead.

```
[kernel] WARN: Backtrace:
[kernel] WARN:     #0: ffff80000034a7e0 backtrace()+0x3c
[kernel] WARN:     #1: ffff800000113de7 kernel_ipc()+0x77
[kernel] WARN:     #2: ffff80000010f7bb mainloop()+0x6b
[kernel] WARN:     #3: ffff80000010f4a9 kernel_server_main()+0x149
[kernel] WARN:     #4: ffff8000001027d6 x64_start_kernel_thread(+0xa
```

## Kernel Debugger
Kernel debugger is available only in the debug build. You can use it over the serial port. Implemented commands are:

- `ps`
  - List processes and threads. It's useful for debugging dead locks.
- `st`
  - Show statistics.


## Wireshark
Use Wireshark to inspect messages!

### Set Up
1. Install this dissector (in macOS, copy this file to ~/.config/wireshark/plugins/).
2. Open `Prefrences > Protocols > DLT_USER > Encapsulations Table`.
3. Add a new rule: `DLT="User 0 (DLT=147)", Payload protocol="resea", Header/Trailer size=0`
   (see [HowToDissectAnything - The Wireshark Wiki](https://wiki.wireshark.org/HowToDissectAnything)).

### How to Use
1. Edit `kernel/ipc.c` to enable `DUMP_MESSAGE` (commented out by default).
2. Run Resea, extract messages, and open Wireshark:
```
$ make run | tee boot.log
$ grep "pcap>" boot.log > messages.log
$ text2pcap -l 147 messages.log messages.pcap
$ wireshark messages.pcap
```

## Runtime Checkers
In the debug build, the following runtime checkers are enabled.
- Kernel Stack Canary
  - Detects too much kernel stack consumption.
- [Undefined Behavior Sanitizer (UBSan)](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
  - Detects undefined behaviors like integer overflow.
- [Kernel Address Sanitizer (KASan)](https://clang.llvm.org/docs/AddressSanitizer.html) *(work-in-progress)*
  - Detects double-free, NULL pointer dereferences, uninitialized reads, etc.

**Example output:**
```
[kernel] PANIC: asan: detected double-free bug (free_list=ffff8000013f5800)
[kernel] WARN: Backtrace:
[kernel] WARN:     #0: ffff80000034caa1 backtrace()+0x84
[kernel] WARN:     #1: ffff80000034ee63 kasan_check_double_free()+0x83
[kernel] WARN:     #2: ffff80000010c507 add_free_list()+0x27
[kernel] WARN:     #3: ffff80000034da32 debugger_run()+0x7b2
[kernel] WARN:     #4: ffff80000034d14b debugger_oninterrupt()+0xcb
[kernel] WARN:     #5: ffff800000102979 x64_handle_interrupt()+0xd9
[kernel] WARN:     #6: ffff800000102820 handle_interrupt()+0x30
[kernel] WARN:     #7: ffff80000010103a halt()+0x0
```
