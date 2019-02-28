#include <arch.h>
#include <x64/gdt.h>
#include <x64/cpu.h>
#include <x64/asm.h>

void x64_gdt_init(void) {
    CPUVAR->gdt[GDT_NULL]        = 0x0000000000000000;
    CPUVAR->gdt[GDT_KERNEL_CODE] = 0x00af9a000000ffff;
    CPUVAR->gdt[GDT_KERNEL_DATA] = 0x00af92000000ffff;
    CPUVAR->gdt[GDT_USER_CODE32] = 0x0000000000000000;
    CPUVAR->gdt[GDT_USER_CODE64] = 0x00affa000000ffff;
    CPUVAR->gdt[GDT_USER_DATA]   = 0x008ff2000000ffff;

    uint64_t tss_addr = (uint64_t) &CPUVAR->tss;
    CPUVAR->gdt[GDT_TSS] = 0x0000890000000067
        | ((tss_addr & 0xffff) << 16)
        | (((tss_addr >> 16) & 0xff) << 32)
        | (((tss_addr >> 24) & 0xff) << 56);
    CPUVAR->gdt[GDT_TSS + 1] = tss_addr >> 32;

    // Update GDTR
    CPUVAR->gdtr.laddr = (uint64_t) &CPUVAR->gdt;
    CPUVAR->gdtr.len = sizeof(uint64_t) * GDT_DESC_NUM - 1;
    asm_lgdt((uint64_t) &CPUVAR->gdtr);
}
