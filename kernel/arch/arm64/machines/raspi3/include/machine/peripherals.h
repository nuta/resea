#ifndef __ARM_PERIPHERALS_H__
#define __ARM_PERIPHERALS_H__

#include <arch.h>

// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
// TODO: Let me know if you know the location of the BCM2837 datasheet!

#define MMIO_BASE      0x3f000000
#define UART0_DR        (MMIO_BASE + 0x00201000)  // Data register.
#define UART0_FR        (MMIO_BASE + 0x00201018)  // Flag register.
#define UART0_IBRD      (MMIO_BASE + 0x00201024)  // Integer baud rate divisor.
#define UART0_FBRD      (MMIO_BASE + 0x00201028)  // Fractional part of the baud rate divisor.
#define UART0_LCRH      (MMIO_BASE + 0x0020102c)  // Line control registe.
#define UART0_CR        (MMIO_BASE + 0x00201030)  // Control register.
#define UART0_ICR       (MMIO_BASE + 0x00201044)  // Interrupt clear register.
#define GPIO_FSEL1      (MMIO_BASE + 0x00200004)  // GPIO Function Select 1.
#define GPIO_PUD        (MMIO_BASE + 0x00200094)  // GPIO Pin Pull-up/down Enable.
#define GPIO_PUDCLK0    (MMIO_BASE + 0x00200098)  // GPIO Pin Pull-up/down Enable Clock 0.
#define PM_RSTC         (MMIO_BASE + 0x0010001c)
#define PM_WDOG         (MMIO_BASE + 0x00100024)
#define MBOX_READ       (MMIO_BASE + 0xb880 + 0x0)
#define MBOX_POLL       (MMIO_BASE + 0xb880 + 0x10)
#define MBOX_SENDER     (MMIO_BASE + 0xb880 + 0x14)
#define MBOX_STATUS     (MMIO_BASE + 0xb880 + 0x18)
#define MBOX_CONFIG     (MMIO_BASE + 0xb880 + 0x1C)
#define MBOX_WRITE      (MMIO_BASE + 0xb880 + 0x20)

//
//  Mailbox - An interface to communicate with VideoCore (GPU) on Raspberry Pi.
//  https://github.com/raspberrypi/firmware/wiki/Mailboxes
//

// Mailbox channels.
#define MBOX_CH_PROP    8

// Mailbox tags.
#define MBOX_TAG_SETCLKRATE  0x38002
#define MBOX_TAG_LAST        0

#define MBOX_CODE_REQUEST 0
#define MBOX_FULL    0x80000000
#define MBOX_EMPTY   0x40000000

// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
#define CORE0_TIMER_IRQCNTL 0x40000040

// Watchdog timer values.
#define PM_PASSWORD        0x5a000000
#define PM_WDOG_FULL_RESET 0x00000020

static inline uint32_t mmio_read(vaddr_t paddr) {
    return *((volatile uint32_t *) from_paddr(paddr));
}

static inline void mmio_write(paddr_t paddr, uint32_t value) {
    *((volatile uint32_t *) from_paddr(paddr)) = value;
}

void arm64_peripherals_init(void);
void arm64_timer_reload(void);

#endif
