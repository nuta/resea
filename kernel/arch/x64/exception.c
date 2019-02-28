#include <types.h>
#include <printk.h>
#include <x64/asm.h>
#define EXP_PAGE_FAULT 14

struct regs {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t error;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} PACKED;

static void print_regs(struct regs *regs) {
    INFO("RIP = %p    CS  = %p    RFLAGS = %p", regs->rip, regs->cs, regs->rflags);
    INFO("SS  = %p    RSP = %p    RBP = %p", regs->ss, regs->rsp, regs->rbp);
    INFO("RAX = %p    RBX = %p    RCX = %p", regs->rax, regs->rbx, regs->rcx);
    INFO("RDX = %p    RSI = %p    RDI = %p", regs->rdx, regs->rsi, regs->rdi);
    INFO("R8  = %p    R9  = %p    R10 = %p", regs->r8, regs->r9, regs->r10);
    INFO("R11 = %p    R12 = %p    R13 = %p", regs->r11, regs->r12, regs->r13);
    INFO("R14 = %p    R15 = %p", regs->r14, regs->r15);
}

void x64_handle_exception(uint8_t exc, struct regs *regs) {
    switch (exc) {
        case EXP_PAGE_FAULT: {
            vaddr_t addr = asm_read_cr2();
            INFO("#PF: addr=%p, error=%d", addr, regs->error);
            print_regs(regs);

            if ((regs->error >> 3) & 1) {
                PANIC("#PF: RSVD bit violation");
            }

            page_fault_handler(addr, regs->error);
            break;
        }
        default:
            INFO("Exception #%d", exc);
            print_regs(regs);
            PANIC("Unhandled exception #%d", exc);
    }
}
