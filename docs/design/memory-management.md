# Memory Management

Resea Kernel does not allocate memory for userland programs. Instead, an
userland server task manages memory pages. The kernel only reserves small
chunk of memory for its internal use (e.g. page tables).

## How Page Faults are Handled
1. Page fault occurrs in a task.
2. The kernel sends a `PAGE_FAULT_MSG` to its pager task on behalf of the task.
3. The pager task (e.g. `bootstrap` server) allocates and fills a memory page
   and returns its physical memory address.
4. The kernel updates the page table and resumes the task.
