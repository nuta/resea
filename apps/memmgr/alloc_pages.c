#include "alloc_pages.h"

uintptr_t alloc_pages(size_t num) {
    static uintptr_t next_addr = 0x04000000;
    uintptr_t addr = next_addr;
    next_addr += num * 4096;
    return addr;
}
