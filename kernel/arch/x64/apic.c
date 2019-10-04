#include <arch.h>
#include <printk.h>
#include <x64/apic.h>
#include <x64/x64.h>

#define PIT_HZ 1193182
#define PIT_CH2 0x42
#define PIT_CMD 0x43
#define KBC_PORT_B 0x61
#define KBC_B_OUT2_STATUS 0x20
#define APIC_TIMER_DIV 0x03

static paddr_t ioapic_paddr;

static inline uint32_t x64_read_apic(paddr_t addr) {
    return *((volatile uint32_t *) from_paddr(addr));
}

static inline void x64_write_apic(paddr_t addr, uint32_t data) {
    *((volatile uint32_t *) from_paddr(addr)) = data;
}

static uint32_t ioapic_read(uint8_t reg) {
    *((uint32_t *) from_paddr(ioapic_paddr)) = reg;
    return *((uint32_t *) from_paddr(ioapic_paddr + 0x10));
}

static void ioapic_write(uint8_t reg, uint32_t data) {
    *((uint32_t *) from_paddr(ioapic_paddr)) = reg;
    *((uint32_t *) from_paddr(ioapic_paddr + 0x10)) = data;
}

static void x64_ioapic_enable_irq(uint8_t vector, uint8_t irq) {
    ioapic_write(IOAPIC_REG_NTH_IOREDTBL_HIGH(irq), 0);
    ioapic_write(IOAPIC_REG_NTH_IOREDTBL_LOW(irq), vector);
}

void enable_irq(uint8_t irq) {
    x64_ioapic_enable_irq(VECTOR_IRQ_BASE + irq, irq);
}

void x64_send_ipi(uint8_t vector, enum ipi_dest dest, uint8_t dest_apic_id,
                  enum ipi_mode mode) {
    uint64_t data =
          ((uint64_t) dest_apic_id << 56)
        | ((uint64_t) dest << 18)
        | (1ULL << 14) // Level: assert
        | ((uint64_t) mode << 8)
        | vector;
    x64_write_apic(APIC_REG_ICR_HIGH, data >> 32);
    x64_write_apic(APIC_REG_ICR_LOW, data & 0xffffffff);
    INFO("sent IPI: %p", data);
}

void x64_ioapic_init(paddr_t ioapic_addr) {
    int max;
    ioapic_paddr = ioapic_addr;

    // symmetric I/O mode
    asm_out8(0x22, 0x70);
    asm_out8(0x23, 0x01);

    // get the maxinum number of entries in IOREDTBL
    max = (int) (ioapic_read(IOAPIC_REG_IOAPICVER) >> 16) + 1;

    // disable all hardware interrupts
    for (int i = 0; i < max; i++) {
        ioapic_write(IOAPIC_REG_NTH_IOREDTBL_HIGH(i), 0);
        ioapic_write(IOAPIC_REG_NTH_IOREDTBL_LOW(i), 1 << 16 /* masked */);
    }
}

static void calibrate_apic_timer(void) {
    // FIXME: This method works well on QEMU though on the real machine
    // this doesn't. For example on my machine `counts_per_sec' is 7x greater
    // than the desired value.

    // initialize PIT
    int freq = 1000; /* That is, every 1ms. */
    uint16_t divider = PIT_HZ / freq;
    asm_out8(KBC_PORT_B,
        asm_in8(KBC_PORT_B) & 0xfc /* Disable ch #2 and speaker output */);
    asm_out8(PIT_CMD, 0xb2 /* Select ch #2 */);
    asm_out8(PIT_CH2, divider & 0xff);
    asm_out8(PIT_CH2, (divider >> 8) & 0xff);
    asm_out8(KBC_PORT_B, asm_in8(KBC_PORT_B) | 1 /* Enable ch #2 */);

    // Reset the counter in APIC timer.
    uint64_t init_count = 0xffffffff;
    x64_write_apic(APIC_REG_TIMER_INITCNT, init_count);

    // Wait for the PIT.
    while ((asm_in8(KBC_PORT_B) & KBC_B_OUT2_STATUS) == 0) {}

    uint64_t diff = init_count - x64_read_apic(APIC_REG_TIMER_CURRENT);
    uint32_t counts_per_sec = (diff * freq) << APIC_TIMER_DIV;
    x64_write_apic(APIC_REG_TIMER_INITCNT, counts_per_sec / TICK_HZ);
    TRACE("calibrated the APIC timer using PIT: %d counts/msec",
        counts_per_sec / 1000);
}

void x64_ack_interrupt(void) {
    x64_write_apic(APIC_REG_EOI, 0);
}

void x64_apic_timer_init(void) {
    x64_ioapic_enable_irq(APIC_TIMER_VECTOR, 0);
    x64_write_apic(APIC_REG_TIMER_INITCNT, 0xffffffff);
    x64_write_apic(APIC_REG_LVT_TIMER, APIC_TIMER_VECTOR | 0x20000);
    x64_write_apic(APIC_REG_TIMER_DIV, APIC_TIMER_DIV);
    calibrate_apic_timer();
}

void x64_apic_init(void) {
    // Local APIC hardware intialization
    asm_wrmsr(MSR_APIC_BASE, (asm_rdmsr(MSR_APIC_BASE) & 0xfffff100) | 0x0800);

    // Local APIC software initialization
    x64_write_apic(APIC_REG_SPURIOUS_INT, 1 << 8);

    // task priority
    x64_write_apic(APIC_REG_TPR, 0);

    // logical destination register
    x64_write_apic(APIC_REG_LOGICAL_DEST, 0x01000000);

    // destination format register
    x64_write_apic(APIC_REG_DEST_FORMAT, 0xffffffff);

    // timer interrupt
    x64_write_apic(APIC_REG_LVT_TIMER, 1 << 16 /* masked */);

    // errror interrupt
    x64_write_apic(APIC_REG_LVT_ERROR, 1 << 16 /* masked */);
}
