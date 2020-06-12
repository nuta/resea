#ifndef __ARM_PERIPHERALS_H__
#define __ARM_PERIPHERALS_H__

#include <arch.h>

// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
// TODO: Let me know if you know the location of the BCM2837 datasheet!
#define UART0_BASE 0x3f201000
#define UART0_DR   (UART0_BASE + 0x0000)
#define UART0_FR   (UART0_BASE + 0x0018)

// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
#define CORE0_TIMER_IRQCNTL 0x40000040

static inline uint32_t mmio_read(vaddr_t paddr) {
    return *((volatile uint32_t *) from_paddr(paddr));
}

static inline void mmio_write(paddr_t paddr, uint32_t value) {
    *((volatile uint32_t *) from_paddr(paddr)) = value;
}

void arm64_peripherals_init(void);
void arm64_timer_reload(void);

#endif
