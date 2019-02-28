#include <arch.h>
#include <x64/asm.h>
#include <x64/cpu.h>
#include <x64/ioapic.h>

static uint32_t ioapic_read(uint8_t reg) {
    *((uint32_t *) from_paddr(CPUVAR->ioapic)) = reg;
    return *((uint32_t *) from_paddr(CPUVAR->ioapic + 0x10));
}


static void ioapic_write(uint8_t reg, uint32_t data) {
    *((uint32_t *) from_paddr(CPUVAR->ioapic)) = reg;
    *((uint32_t *) from_paddr(CPUVAR->ioapic + 0x10)) = data;
}


void x64_ioapic_enable_irq(uint8_t vector, uint8_t irq) {
    ioapic_write(IOAPIC_REG_NTH_IOREDTBL_HIGH(irq), 0);
    ioapic_write(IOAPIC_REG_NTH_IOREDTBL_LOW(irq),  vector);
}


void x64_ioapic_init(paddr_t ioapic_addr) {
    int max;
    CPUVAR->ioapic = ioapic_addr;

    // symmetric I/O mode
    asm_out8(0x22, 0x70);
    asm_out8(0x23, 0x01);

    // get the maxinum number of entries in IOREDTBL
    max = (ioapic_read(IOAPIC_REG_IOAPICVER) >> 16) + 1;

    // disable all hardware interrupts
    for (int i=0; i < max; i++) {
        ioapic_write(IOAPIC_REG_NTH_IOREDTBL_HIGH(i), 0);
        ioapic_write(IOAPIC_REG_NTH_IOREDTBL_LOW(i),  1 << 16 /* masked */);
    }
}
