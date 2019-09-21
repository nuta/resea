#ifndef __DRIVER_IO_X64_H__
#define __DRIVER_IO_X64_H__

#include <types.h>

static inline uint8_t arch_ioport_read8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t arch_ioport_read16(uint16_t port) {
    uint16_t value;
    __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t arch_ioport_read32(uint16_t port) {
    uint32_t value;
    __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void arch_ioport_write8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" ::"a"(value), "Nd"(port));
}

static inline void arch_ioport_write16(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" ::"a"(value), "Nd"(port));
}

static inline void arch_ioport_write32(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" ::"a"(value), "Nd"(port));
}


#endif
