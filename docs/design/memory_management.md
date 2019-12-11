# Memory Management
Resea Kernel does not allocate memory pages for userland. It delegates all
physical memory pages except kernel memory to the first userland process (`memmgr`).

## Kernel Memory Allocator
Resea Kernel reserves fixed-length physical memory space for allocating in-kernel
data structures: thread struct, page tables, etc. It does not provide any interface
to allocate memory for userland processes.

## Userland Memory Allocator (`memmgr`)
`memmgr`, the first userland process owns all available physical memory pages.
It allocates memory pages for userland applications on behalf of the kernel
in order to [separate memory allocation policies](https://en.wikipedia.org/wiki/Separation_of_mechanism_and_policy) from the kernel.

## Pagers (or How Page Faults are Handled)
When a page fault occurr, the kernel searches for appropriate Virtual Memory
Area (VMA) for the page fault address and invokes the VMA's pager. A *pager*
is responsible for filling the requested page and there're three types of pagers:

- *Initfs Pager* fills the [initfs](../internals/initfs.md) image, a file system
  embedded into the kernel.
- *Straight Mapping Pager* simply maps the page fault (virtual) address to the
  the physical memory address (so-called *straight mapping*). It's used only
  in the memmgr process.
- *User Pager* calls (send a request and waits for a reply) a channel to
  request filling a page.
