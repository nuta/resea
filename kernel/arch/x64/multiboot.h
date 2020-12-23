#ifndef __X64_MULTIBOOT_H__
#define __X64_MULTIBOOT_H__

#include <types.h>

struct __packed multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint8_t syms[16];
    uint32_t mmap_len;
    uint32_t mmap_addr;
};

struct __packed multiboot_mmap_entry {
    uint32_t entry_size;
    uint64_t base;
    uint64_t len;
    uint32_t type;
};

#endif