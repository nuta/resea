memmgr - The user-level physical memory manager
===============================================

Memmgr is the first process in the userland and provides some crucial
features:

- Physical memory pages manamgement. The kernel delegates all physical pages
  to memmgr other than kernel memory area.
- Spawning startup servers. Memmgr loads and executes ELF executable files in
  initfs `startup` directory.
- Discovery service. startup servers uses this service for service discovery.
- Initfs fs server.

