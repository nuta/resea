#ifndef __ARCH_IO_H__
#define __ARCH_IO_H__

#include <types.h>

static inline void memory_barrier(void) {
    __sync_synchronize();
}

// Disable clang-format temporarily because it does not handle "::" as desired.
// clang-format off
static inline void io_out8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline void io_out16(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" :: "a"(value), "Nd"(port));
}

static inline void io_out32(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" :: "a"(value), "Nd"(port));
}
// clang-format on

static inline uint8_t io_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t io_in16(uint16_t port) {
    uint16_t value;
    __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t io_in32(uint16_t port) {
    uint32_t value;
    __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

#endif
