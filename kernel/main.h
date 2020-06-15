#ifndef __MAIN_H__
#define __MAIN_H__

struct bootelf_mapping {
    uint64_t vaddr;
    uint32_t offset;
    uint16_t num_pages;
    uint8_t type;   // 'R': readonly, 'W': writable, 'X': executable.
    uint8_t zeroed; // If it's non-zero value, the pages are filled with zeros.
} PACKED;

struct bootelf_header {
#define BOOTELF_MAGIC "11BOOT\xe1\xff"
    uint8_t magic[8];
    uint8_t name[16];
    uint64_t entry;
    uint8_t num_mappings;
    uint8_t padding[7];
    struct bootelf_mapping mappings[];
} PACKED;

void kmain(void);
NORETURN void mpmain(void);

// Implemented in arch.
void mp_start(void);
NORETURN void arch_idle(void);
void halt(void);

#endif
