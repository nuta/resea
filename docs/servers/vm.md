# Virtual Memory Manager (vm)
The virtual memory manager, called `vm`, is responsible for allocating physical memory pages and managing tasks (as a pager task), etc.

Once the kernel initializes itself, it loads and starts `vm` as the first task.

## Features
- Allocating and mapping physical memory pages. In other words, the kernel does *not* allocate memory pages at all. The responsibility is delegated to vm.
- Launching tasks and handling their exceptions (e.g. page faults) as their pager task.
- Service discovery (`ipc_lookup` API).
- [Out-of-Line payload](../userspace/ool) transmitting.


## Source Location
[servers/vm](https://github.com/nuta/resea/tree/master/servers/vm)
