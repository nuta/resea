#include <boot.h>
#include <x64/gdt.h>
#include <x64/pic.h>
#include <x64/apic.h>
#include <x64/tss.h>
#include <x64/idt.h>
#include <x64/smp.h>
#include <x64/print.h>
#include <x64/syscall.h>
#include <x64/setup.h>

void x64_setup(void) {
    x64_print_init();
    boot();
}

void arch_init(void) {
    x64_pic_init();
    x64_apic_init();
    x64_gdt_init();
    x64_tss_init();
    x64_idt_init();
    x64_smp_init();
    x64_apic_timer_init();
    x64_syscall_init();
}
