#include <types.h>
#include <config.h>
#include <task.h>
#include <syscall.h>
#include <printk.h>
#include <machine/peripherals.h>
#include "asm.h"

// Defined in usercopy.S.
extern char arm64_usercopy1[];
extern char arm64_usercopy2[];
extern char arm64_usercopy3[];

void arm64_handle_interrupt(void) {
    arm64_timer_reload();
    handle_timer_irq();
}

void arm64_handle_exception(void) {
    uint64_t esr = ARM64_MRS(esr_el1);
    uint64_t elr = ARM64_MRS(elr_el1);
    uint64_t far = ARM64_MRS(far_el1);
    unsigned ec = esr >> 26;
    switch (ec) {
        // SVC.
        case 0x15:
            // Handled in interrupt_common.
            UNREACHABLE();
            break;
        // Instruction Abort in userspace (page fault).
        case 0x20:
#ifdef CONFIG_TRACE_EXCEPTION
            TRACE("Instruction Abort: task=%s, far=%p, elr=%p, esr=%p",
                  CURRENT->name, far, elr, esr);
#endif
            handle_page_fault(far, elr, EXP_PF_USER | EXP_PF_WRITE /* FIXME: */);
            break;
        // Data abort in userspace (page fault).
        case 0x24:
#ifdef CONFIG_TRACE_EXCEPTION
            TRACE("Data Abort: task=%s, far=%p, elr=%p, esr=%p",
                  CURRENT->name, far, elr, esr);
#endif
            handle_page_fault(far, elr, EXP_PF_USER | EXP_PF_WRITE /* FIXME: */);
            break;
        // Data abort in kernel.
        case 0x25:
#ifdef CONFIG_TRACE_EXCEPTION
             TRACE("Data Abort (kernel): far=%p, elr=%p", far, elr);
#endif
            if (elr != (vaddr_t) arm64_usercopy1
                && elr != (vaddr_t) arm64_usercopy2) {
                PANIC("page fault in the kernel: task=%s, far=%p, elr=%p",
                      CURRENT->name, far, elr);
            }

            handle_page_fault(far, elr, EXP_PF_USER | EXP_PF_WRITE /* FIXME: */);
            break;
        default:
            PANIC("unknown exception: ec=%d (0x%x), elr=%p, far=%p",
                  ec, ec, elr, far);
    }
}
