#ifndef __X64_MULTIBOOT_H__
#define __X64_MULTIBOOT_H__

#include <types.h>

struct elf_section_header {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t index;
} PACKED;

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t num_modules;
    uint32_t modules_paddr;
    struct elf_section_header section;
    uint32_t mmap_len;
    uint32_t mmap_paddr;
} PACKED;

struct multiboot_mmap {
    uint32_t size_of_struct;
    uint64_t addr;
    uint64_t len;
#define MULTIBOOT_MMAP_FREE      1
#define MULTIBOOT_MMAP_RESERVED  2
#define MULTIBOOT_MMAP_ACPI      3
#define MULTIBOOT_MMAP_NVS       4
#define MULTIBOOT_MMAP_BAD_RAM   5
    uint32_t type;
} PACKED;

#endif
