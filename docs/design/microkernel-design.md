# Microkernel Design
Microkernel, the key concept of Resea, is a well-studied operating system kernel
design which extracts and isolates traditional kernel features such as device
drivers and TCP/IP protocol stack into userspace.

## Pros and Cons
The microkernel design improves flexibility and security by isolating
components.

However, it has been said that microkernels are slow compared to monolithic
kernels because communication between components involves Inter Process
Communication (IPC) via kernel.

Improving IPC performance has been a key research topic: see
[L4 microkernel familly](https://doi.org/10.1145/2517349.2522720) and
[SkyBridge](https://doi.org/10.1145/3302424.3303946) if you're interested in.

## Microkernel-based Design in Resea
Resea's microkernel (Resea Kernel) aims to be *pure*: the kernel does not
implement features that can be done in userspace.

In Resea, like other microkernel-based operating systems, traditional kernel
features such as file system, TCP/IP, and device drivers are implemented as
standalone userland programs (called *servers*).

Similar to the *"Everything is a file"* philosophy in Unix, Resea has a
philosophy: *"Everything is a message passing"*. Reading a file, receiving a
keyboard event, and all other operations and events are represented as messages.
