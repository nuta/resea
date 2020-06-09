#ifndef __ARM_PERIPHERALS_H__
#define __ARM_PERIPHERALS_H__

#define UART0_BASE 0xffff00003f201000
#define UART0_DR   (UART0_BASE + 0x0000)
#define UART0_FR   (UART0_BASE + 0x0018)

static inline uint32_t mmio_read(vaddr_t addr) {
    return *((volatile uint32_t *) addr);
}

static inline void mmio_write(vaddr_t addr, uint32_t value) {
    *((volatile uint32_t *) addr) = value;
}

void arm_peripherals_init(void);

#endif
