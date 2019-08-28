#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

// See INTERNALS.md for details.
#ifdef __x86_64__
#    define MEMMGR_STACK_END 0x02400000
#    define MEMMGR_BSS_START 0x02400000
#    define FREE_MEMORY_START 0x03000000
#    define THREAD_INFO_ADDR 0x0000000000000f1b000
#    define APP_IMAGE_START 0x01000000
#    define APP_IMAGE_SIZE 0x01000000
#    define APP_ZEROED_PAGES_START 0x02000000
#    define APP_ZEROED_PAGES_SIZE 0x02000000
#    define APP_INITIAL_STACK_POINTER 0x03000000
#else
#    error "unsupported CPU architecture"
#endif

#endif
