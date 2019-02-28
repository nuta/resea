#include <memory.h>
#include <printk.h>
#include <x64/tss.h>
#include <x64/asm.h>
#include <x64/cpu.h>

void x64_tss_init(void) {
    vaddr_t intr_stack = (vaddr_t) alloc_page();
    if (!intr_stack) {
        PANIC("failed to allocate a stack");
    }

    vaddr_t exp_stack = (vaddr_t) alloc_page();
    if (!exp_stack) {
        PANIC("failed to allocate a stack");
    }

    struct tss *tss = &CPUVAR->tss;
    tss->ist[INTR_HANDLER_IST - 1] = intr_stack + PAGE_SIZE;
    tss->ist[EXP_HANDLER_IST  - 1] = exp_stack + PAGE_SIZE;
    tss->iomap = sizeof(struct tss);
    asm_ltr(TSS_SEG);
}
