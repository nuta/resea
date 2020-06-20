# Kernel Debugging
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

## Runtime Checkers
In the debug build, the following runtime checkers are enabled.
- Kernel Stack Canary
  - Detects too much kernel stack consumption.
- [Undefined Behavior Sanitizer (UBSan)](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
  - Detects undefined behaviors like integer overflow.
