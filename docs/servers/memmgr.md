# Memory Manager (memmgr)
`memmgr` is the first process in the userspace. It manages all available physical
memory pages delegated from the kernel except kernel memory. In addition to
user-level memory management, it provides some other critical features.

## Features
- Spawning servers from initfs and handling page faults in their processes.
  - Spawned servers has two channels setted up by memmgr:
    - `@1`: Kernel server
    - `@2`: memmgr
- Service discovery (`discovery` interface).
- Allocating physical memory pages to userland programs
  (so-called *user-level memory mangement*).

## Implements
- `memmgr`
- `pager`
- `discovery`
- `runtime`
- `timer`

## Depends on
- `kernel`
