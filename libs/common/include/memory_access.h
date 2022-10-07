#pragma once
#include <types.h>

#define MEMORY_BARRIER() __sync_synchronize()

static inline void volatile_write8(vaddr_t addr, uint8_t val) {
    *((volatile uint8_t *) addr) = val;
}

static inline void volatile_write16(vaddr_t addr, uint16_t val) {
    *((volatile uint16_t *) addr) = val;
}

static inline void volatile_write32(vaddr_t addr, uint32_t val) {
    *((volatile uint32_t *) addr) = val;
}

static inline uint8_t volatile_read8(vaddr_t addr) {
    return *((volatile uint8_t *) addr);
}

static inline uint16_t volatile_read16(vaddr_t addr) {
    return *((volatile uint16_t *) addr);
}

static inline uint32_t volatile_read32(vaddr_t addr) {
    return *((volatile uint32_t *) addr);
}
