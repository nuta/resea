#include <types.h>
#include <printk.h>
#include <machine/peripherals.h>
#include "asm.h"

static inline void delay(unsigned clocks) {
    while (clocks-- > 0) {
        __asm__ __volatile__("nop");
    }
}

/// Performs a mailbox call.
static void mbox_call(uint8_t ch, const uint32_t *mbox) {
    ASSERT(IS_ALIGNED((vaddr_t) mbox, 0x10));

    while (mmio_read(MBOX_STATUS) & MBOX_FULL) {
        delay(1);
    }

    // Send a message.
    unsigned int value =
        (((vaddr_t) mbox) & ~0xf) // The address of the mailbox to be sent.
        | (ch & 0xf)              // Channel.
        ;
    mmio_write(MBOX_WRITE, value);

    // Wait for the response...
    while (1) {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) {
            delay(1);
        }

        if (value == mmio_read(MBOX_READ)) {
            break;
        }
    }
}

void uart_init(void) {
    // Set up the clock.
    uint32_t __aligned(16) m[9];
    m[0] = 9*4;                  // The size of the meessage.
    m[1] = MBOX_CODE_REQUEST;
    m[2] = MBOX_TAG_SETCLKRATE;  // Set clock rate.
    m[3] = 12;                   // The value length.
    m[4] = 8;                    // The clock ID.
    m[5] = 2;                    // UART clock
    m[6] = 4000000;              // The clock rate = 4MHz. (TODO: is 4MHz too low?)
    m[7] = 0;                    // Allow turbo settings.
    m[8] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP, m);

    // Initialize GPIO pins #14 and #15 as alternative function 0 (UART0).
    mmio_write(GPIO_FSEL1,
        (mmio_read(GPIO_FSEL1) & ~((0b111 << 12)|(0b111 << 15)))
        | (0b100 << 12) | (0b100 << 15));

    // See "GPIO Pull-up/down Clock Registers (GPPUDCLKn)" in "BCM2835 ARM Peripherals"
    // for more details on this initializaiton sequence.
    mmio_write(GPIO_PUD, 0);
    delay(150);
    mmio_write(GPIO_PUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    mmio_write(GPIO_PUDCLK0, 0);

    mmio_write(UART0_CR, 0);            // Disable UART.
    mmio_write(UART0_ICR, 0x7ff);       // Disable interrupts from UART.
    mmio_write(UART0_IBRD, 2);          // baud rate = 115200
    mmio_write(UART0_FBRD, 11);         //
    mmio_write(UART0_LCRH, 0b11 << 5);  // 8n1
    mmio_write(UART0_CR, 0x301);        // Enable RX, TX, and UART0.
}

bool kdebug_is_readable(void) {
    return (mmio_read(UART0_FR) & (1 << 4)) == 0;
}

int kdebug_readchar(void) {
    if (!kdebug_is_readable()) {
        return -1;
    }

    return mmio_read(UART0_DR);
}

void arch_printchar(char ch) {
    while (mmio_read(UART0_FR) & (1 << 5));
    mmio_write(UART0_DR, ch);
}

#ifdef CONFIG_FORCE_REBOOT_BY_WATCHDOG
static void watchdog_init(void) {
	mmio_write(PM_WDOG, PM_PASSWORD | CONFIG_WATCHDOG_TIMEOUT << 16);
	mmio_write(PM_RSTC, PM_PASSWORD | PM_WDOG_FULL_RESET);
}
#endif

void arm64_timer_reload(void) {
    uint64_t hz = ARM64_MRS(cntfrq_el0);
    ASSERT(hz >= 1000);
    hz /= 1000;
    ARM64_MSR(cntv_tval_el0, hz);
}

static void timer_init(void) {
    arm64_timer_reload();
    ARM64_MSR(cntv_ctl_el0, 1ull);
    mmio_write(TIMER_IRQCNTL(mp_self()), 1 << 3 /* Enable nCNTVIRQ IRQ */);
}

void arm64_peripherals_init(void) {
    uart_init();
#ifdef CONFIG_FORCE_REBOOT_BY_WATCHDOG
    watchdog_init();
#endif
    timer_init();
}

void arch_enable_irq(unsigned irq) {
    // TODO:
}

void arch_disable_irq(unsigned irq) {
    // TODO:
}
