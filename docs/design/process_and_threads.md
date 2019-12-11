# Processes and Threads
Processes and Threads in Resea and them in modern UNIX-like operating systems
are same concepts.

In Resea, a process and a thread have the following data structures respectively:

## Process
- Process ID
- Process Name
- Threads
- Channels
- Page Table
- Virtual Memory Areas (VMA)
  - The start address, the length, allowed operations (read/write/execute),
    and the pager.

## Thread
- Thread ID
- Thread State (whether it's runnable or waits for a message)
- CPU execution context (registers)
- Kenel stack
- Thread-local Information Block
  - Page Base (the destination virtual address for received a page payload)
  - User IPC buffer
- Kernel IPC buffer
  - Used when the kernel needs to send a message on behalf of the thread
    (e.g., user pager).

