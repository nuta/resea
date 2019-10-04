#ifndef __X64_APIC_H__
#define __X64_APIC_H__

#include <memory.h>
#include <types.h>

#define APIC_REG_ID 0xfee00020
#define APIC_REG_VERSION 0xfee00030
#define APIC_REG_TPR 0xfee00080
#define APIC_REG_EOI 0xfee000b0
#define APIC_REG_LOGICAL_DEST 0xfee000d0
#define APIC_REG_DEST_FORMAT 0xfee000e0
#define APIC_REG_SPURIOUS_INT 0xfee000f0
#define APIC_REG_ICR_LOW 0xfee00300
#define APIC_REG_ICR_HIGH 0xfee00310
#define APIC_REG_LVT_TIMER 0xfee00320
#define APIC_REG_LINT0 0xfee00350
#define APIC_REG_LINT1 0xfee00360
#define APIC_REG_LVT_ERROR 0xfee00370
#define APIC_REG_TIMER_INITCNT 0xfee00380
#define APIC_REG_TIMER_CURRENT 0xfee00390
#define APIC_REG_TIMER_DIV 0xfee003e0
#define AP_BOOT_CODE_PADDR  0x5000

#define IOAPIC_IOREGSEL_OFFSET 0x00
#define IOAPIC_IOWIN_OFFSET 0x10
#define VECTOR_IRQ_BASE 32
#define APIC_TIMER_VECTOR (VECTOR_IRQ_BASE + 0)
#define IOAPIC_REG_IOAPICVER 0x01
#define IOAPIC_REG_NTH_IOREDTBL_LOW(n) (0x10 + (n * 2))
#define IOAPIC_REG_NTH_IOREDTBL_HIGH(n) (0x10 + (n * 2) + 1)

enum ipi_dest {
    IPI_DEST_UNICAST            = 0,
    IPI_DEST_SELF               = 1,
    IPI_DEST_ALL_EXCLUDING_SELF = 2,
    IPI_DEST_ALL                = 3,
};

enum ipi_mode {
    IPI_MODE_FIXED   = 0,
    IPI_MODE_INIT    = 5,
    IPI_MODE_STARTUP = 6,
};

void x64_ioapic_init(paddr_t ioapic_addr);
void x64_apic_init(void);
void x64_apic_timer_init(void);
void x64_ack_interrupt(void);
void x64_send_ipi(uint8_t vector, enum ipi_dest dest, uint8_t dest_apic_id,
                  enum ipi_mode mode);

#endif
