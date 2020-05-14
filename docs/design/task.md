# Tasks
A *task* is a unit of execution just like *process* in other operating systems.
It contains a CPU context (registers) and its own virtual address space.

The biggest difference is that **Resea does not kernel-level threading**:
in other words, only single thread can exist in a task.
It might let you down, however, writing a program in a (single-threaded)
event-driven programming model makes debugging easy[^1].

## Server
*Server* is a task which provides services like device driver, file system,
TCP/IP, etc. We use the term in documentation and code comments but the kernel
does not distinguish between server tasks and client (non-server) tasks.

## Pager
Each tasks (except the very first task created by the kernel) is associated a
*pager*, a task which is responsible for handling exceptions occurred in the
task. When the following events (called *exceptions*) occur, the kernel
communicates with the pager task to handle them:

- **Page fault:** The pager task returns the physical memory address for the
  memory page (or kills the task if it's a so-called segmentation fault).
  For example, a pager allocate a physical memory page for the task, copy the file
  contents into the page, and return its physical address.
- **When a task exits:** Because of invalid opcode exception, divide by zero, etc.
- **ABI Emulation Hook:** If ABI emulation is enabled for the task, the kernel
  asks the pager to handle system calls, etc.

This mechanism is introduced for achiveing [the separation of mechanism and policy](https://en.wikipedia.org/wiki/Separation_of_mechanism_and_policy)
and it suprisingly improves the flexibility of the operating system.

[^1]: [John Ousterhout. Why Threads Are A Bad Idea (for most purposes) [PDF]](https://web.stanford.edu/~ouster/cgi-bin/papers/threads.pdf)
