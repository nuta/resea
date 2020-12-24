#ifndef __BOOTINFO_H__
#define __BOOTINFO_H__

#include <types.h>

#define BOOTINFO_MEMMAP_TYPE_END       0
#define BOOTINFO_MEMMAP_TYPE_AVAILABLE 0xaa
#define BOOTINFO_MEMMAP_TYPE_RESERVED  0xff

struct bootelf_mapping {
    uint64_t vaddr;
    uint32_t offset;
    uint16_t num_pages;
    uint8_t type;    // 'R': readonly, 'W': writable, 'X': executable.
    uint8_t zeroed;  // If it's non-zero value, the pages are filled with zeros.
} __packed;

struct bootinfo_memmap_entry {
    uint8_t type;
    uint8_t reserved[3];
    uint64_t base;
    uint64_t len;
} __packed;

#define NUM_BOOTINFO_MEMMAP_MAX 8

struct bootinfo {
    struct bootinfo_memmap_entry memmap[NUM_BOOTINFO_MEMMAP_MAX];
} __packed;

// The maximum size hard-coded in start.S
STATIC_ASSERT(sizeof(struct bootinfo) <= 512);

#define BOOTELF_MAGIC            "11BOOT\xe1\xff"
#define BOOTELF_NUM_MAPPINGS_MAX 8

struct bootelf_header {
    uint8_t magic[8];
    uint8_t name[16];
    uint64_t entry;
    struct bootelf_mapping mappings[BOOTELF_NUM_MAPPINGS_MAX];
    struct bootinfo bootinfo;
} __packed;

#endif
