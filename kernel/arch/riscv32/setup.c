#include "asm.h"
#include "vm.h"
#include <kernel/arch.h>
#include <kernel/main.h>
#include <kernel/printk.h>
#include <kernel/task.h>

// FIXME:
struct cpuvar cpuvar_fixme;

__noreturn void riscv32_setup_s_mode(void) {
    struct bootinfo bootinfo;
    bootinfo.boot_elf = (paddr_t) __boot_elf;
    bootinfo.memory_map.entries[0].paddr =
        ALIGN_UP((paddr_t) __kernel_image_end, PAGE_SIZE);
    bootinfo.memory_map.entries[0].size = 64 * 1024 * 1024;
    bootinfo.memory_map.entries[0].type = MEMORY_MAP_FREE;
    bootinfo.memory_map.num_entries = 1;

    kernel_main(&bootinfo);
    UNREACHABLE();
}

struct risc32_trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t s0;
    uint32_t s1;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t sp;
    uint32_t tp;
    uint32_t sepc;
    uint32_t sstatus;
} __packed;

void boot_s_mode(void);
void riscv32_trap_handler(void);
void riscv32_timer_handler(void);
void riscv32_trap(struct risc32_trap_frame *frame) {
    uint32_t tp;
    __asm__ __volatile__("mv %0, tp" : "=r"(tp));
    uint32_t sscratch;
    __asm__ __volatile__("csrr %0, sscratch" : "=r"(sscratch));

    uint32_t scause = read_scause();

    TRACE("trap(%s) sepc=%p, scause=%p, stval=%p",
          (read_sstatus() & (1 << 8)) ? "s-mode" : "u-mode", read_sepc(),
          read_scause(), read_stval());

    if (scause == 8) {
        frame->sepc += 4;

        TRACE("syscall: a0=%d, a1=%p, a2=%u", frame->a0, frame->a1, frame->a2);
        for (uint32_t i = 0; i < frame->a2; i++) {
            printf("%c", ((char *) frame->a1)[i]);
        }
    } else if (scause == 0x80000001) {
        write_sip(read_sip() & ~2);
        TRACE("timer context switch!");
        task_switch();
    }
}

__noreturn void riscv32_setup_m_mode(void) {
    write_medeleg(0xffff);
    write_mideleg(0xffff);

    write_stvec((uint32_t) riscv32_trap_handler);
    write_sie(read_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
    write_sscratch((uint32_t) &cpuvar_fixme);
    write_mscratch((uint32_t) &cpuvar_fixme);
    write_tp((uint32_t) &cpuvar_fixme);

    uint32_t mstatus = read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    mstatus |= MSTATUS_SUM;
    mstatus |= 1 << 19;  // FIXME:
    write_mstatus(mstatus);
    write_mepc((uint32_t) boot_s_mode);

    write_satp(0);
    write_pmpaddr0(0xffffffff);
    write_pmpcfg0(0xf);

    int hart = read_mhartid();
    int interval = 1000000;
    CPUVAR->arch.mtimecmp = CLINT_MTIMECMP(hart);
    CPUVAR->arch.mtime = CLINT_MTIME;
    CPUVAR->arch.interval = interval;

    uint32_t *mtimecmp = (uint32_t *) arch_paddr2ptr(CPUVAR->arch.mtimecmp);
    uint32_t *mtime = (uint32_t *) arch_paddr2ptr(CPUVAR->arch.mtime);
    *mtimecmp = *mtime + interval;

    write_mtvec((uint32_t) riscv32_timer_handler);
    write_mstatus(read_mstatus() | MSTATUS_MIE);
    write_mie(read_mie() | MIE_MTIE);

    __asm__ __volatile__("mret");
    UNREACHABLE();
}

void arch_idle(void) {
    DBG("arch_idle");
    write_sstatus(read_sstatus() | SSTATUS_SIE);
    __asm__ __volatile__("wfi");
}
