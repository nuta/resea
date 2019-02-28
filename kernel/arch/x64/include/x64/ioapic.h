#ifndef __X64_IOAPIC_H__
#define __X64_IOAPIC_H__

#include <types.h>

#define IOAPIC_IOREGSEL_OFFSET  0x00
#define IOAPIC_IOWIN_OFFSET     0x10
#define VECTOR_IRQ_BASE 32

#define IOAPIC_REG_IOAPICVER  0x01
#define IOAPIC_REG_NTH_IOREDTBL_LOW(n)  (0x10 + (n * 2))
#define IOAPIC_REG_NTH_IOREDTBL_HIGH(n) (0x10 + (n * 2) + 1)

void x64_ioapic_init(paddr_t ioapic_addr);
void x64_ioapic_enable_irq(uint8_t vector, uint8_t irq);

#endif
