# Microkernel
Microkernel, the key concept of Resea, is a well-studied operating system kernel
design which extracts and isolates traditional kernel features such as device
drivers, file systems, TCP/IP protocol stack, and OS personality into userspace.

It has been said that the microkernel design improves security and reliability
by isolating components but tends to be slow because communication between
components involve Inter Process Communication (IPC) via kernel. Improving
IPC performance has been a key research topic: survey
[L4 microkernel familly](https://doi.org/10.1145/2517349.2522720) and
[SkyBridge](https://doi.org/10.1145/3302424.3303946) if you're interested in.

## Microkernel-based Design in Resea

Resea's microkernel (Resea Kernel) aims to be *pure*, that is, it does not
provide any system call interface except IPC-related operations: *all*
operations are represented as IPC messages.

In Resea, like other microkernel-based operating systems, traditional kernel
features such as file system, TCP/IP, and device drivers, are implemented as
standalone userland programs (*servers*).

Kernel features such as threads are served by *Kernel Server*, a kernel thread
which runs in the kernel.

Similar to the *"Everything is a file"* philosophy in Unix, Resea has a philosophy:
*"Everything is a message passing"*. Want to read a file? Send a message to the
file system server! Want to spawn a new thread? Send a message to the kernel server!
