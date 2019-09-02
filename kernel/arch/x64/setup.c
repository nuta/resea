#include <arch.h>
#include <boot.h>
#include <printk.h>
#include <x64/apic.h>
#include <x64/handler.h>
#include <x64/print.h>
#include <x64/smp.h>
#include <x64/x64.h>

static void cpu_features_init(void) {
    // TODO: Check CPUID.
    asm_write_cr4(asm_read_cr4() | CR4_FSGSBASE);

    //
    //  Initialize FPU.
    //
    CPUVAR->current_fpu_owner = NULL;
    // Enable XSAVE/XRSTOR instruction.
    asm_write_cr4(asm_read_cr4() | CR4_OSXSAVE);
    // Set TS flag to issue an interrupt on use of floating-point registers
    // during the kernel initialization.
    asm_write_cr0(asm_read_cr0() | CR0_TS);
}

static void gdt_init(void) {
    CPUVAR->gdt[GDT_NULL] = 0x0000000000000000;
    CPUVAR->gdt[GDT_KERNEL_CODE] = 0x00af9a000000ffff;
    CPUVAR->gdt[GDT_KERNEL_DATA] = 0x00af92000000ffff;
    CPUVAR->gdt[GDT_USER_CODE32] = 0x0000000000000000;
    CPUVAR->gdt[GDT_USER_CODE64] = 0x00affa000000ffff;
    CPUVAR->gdt[GDT_USER_DATA] = 0x008ff2000000ffff;

    uint64_t tss_addr = (uint64_t) &CPUVAR->tss;
    CPUVAR->gdt[GDT_TSS] = 0x0000890000000067 | ((tss_addr & 0xffff) << 16) |
                           (((tss_addr >> 16) & 0xff) << 32) |
                           (((tss_addr >> 24) & 0xff) << 56);
    CPUVAR->gdt[GDT_TSS + 1] = tss_addr >> 32;

    // Update GDTR
    CPUVAR->gdtr.laddr = (uint64_t) &CPUVAR->gdt;
    CPUVAR->gdtr.len = sizeof(uint64_t) * GDT_DESC_NUM - 1;
    asm_lgdt((uint64_t) &CPUVAR->gdtr);
}

static inline void set_idt_entry(int i, uint64_t handler, uint8_t ist) {
    struct idt_entry *entry = &CPUVAR->idt[i];
    entry->offset1 = handler & 0xffff;
    entry->seg = KERNEL_CS;
    entry->ist = ist;
    entry->info = IDT_INT_HANDLER;
    entry->offset2 = (handler >> 16) & 0xffff;
    entry->offset3 = (handler >> 32) & 0xffffffff;
    entry->reserved = 0;
}

static void idt_init(void) {
    // Initialize IDT entries.
    int handler_size = 16; // FIXME:
    for (int i = 0; i <= 255; i++) {
        set_idt_entry(
            i, (uint64_t) interrupt_handlers + i * handler_size, IST_RSP0);
    }

    CPUVAR->idtr.laddr = (uint64_t) &CPUVAR->idt;
    CPUVAR->idtr.len = (sizeof(struct idt_entry) * IDT_DESC_NUM) + 1;
    asm_lidt((uint64_t) &CPUVAR->idtr);
}

// Disables PIC. We use IO APIC instead.
static void pic_init(void) {

    asm_out8(0xa1, 0xff);
    asm_out8(0x21, 0xff);

    asm_out8(0x20, 0x11);
    asm_out8(0xa0, 0x11);
    asm_out8(0x21, 0x20);
    asm_out8(0xa1, 0x28);
    asm_out8(0x21, 0x04);
    asm_out8(0xa1, 0x02);
    asm_out8(0x21, 0x01);
    asm_out8(0xa1, 0x01);

    asm_out8(0xa1, 0xff);
    asm_out8(0x21, 0xff);
}

static void tss_init(void) {
    struct tss *tss = &CPUVAR->tss;
    // Set RSP0 to 0 until the first context switch so that we can notice a
    // bug which occurs an exception during the boot.
    tss->rsp0 = 0;
    tss->iomap = sizeof(struct tss);
    asm_ltr(TSS_SEG);
}


// SYSRET constraints.
STATIC_ASSERT(USER_CS32 + 8 == USER_DS);
STATIC_ASSERT(USER_CS32 + 16 == USER_CS64);

static void syscall_init(void) {
    asm_wrmsr(
        MSR_STAR, ((uint64_t) USER_CS32 << 48) | ((uint64_t) KERNEL_CS << 32));
    asm_wrmsr(MSR_LSTAR, (uint64_t) x64_syscall_handler);
    asm_wrmsr(MSR_FMASK, SYSCALL_RFLAGS_MASK);
    // RIP for compatibility mode. We don't support it for now.
    asm_wrmsr(MSR_CSTAR, 0);
    // Enable SYSCALL/SYSRET.
    asm_wrmsr(MSR_EFER, asm_rdmsr(MSR_EFER) | EFER_SCE);
}

void x64_setup(void) {
    x64_print_init();
    boot();
}

void arch_init(void) {
    cpu_features_init();
    pic_init();
    x64_apic_init();
    gdt_init();
    tss_init();
    idt_init();
    x64_smp_init();
    x64_apic_timer_init();
    syscall_init();
}

void arch_panic(void) {
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}
