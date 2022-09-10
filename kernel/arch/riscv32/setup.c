#include "asm.h"
#include "debug.h"
#include "mp.h"
#include "plic.h"
#include "task.h"
#include "trap.h"
#include "uart.h"
#include "vm.h"
#include <kernel/arch.h>
#include <kernel/main.h>
#include <kernel/printk.h>
#include <string.h>

// FIXME:
struct cpuvar cpuvar_fixme;

__noreturn void riscv32_setup_s_mode(void) {
    write_sstatus(read_sstatus() | SSTATUS_SUM);
    riscv32_plic_init();

    struct bootinfo bootinfo;
    bootinfo.boot_elf = (paddr_t) __boot_elf;
    bootinfo.memory_map.entries[0].paddr =
        ALIGN_UP((paddr_t) __image_end, PAGE_SIZE);
    bootinfo.memory_map.entries[0].size = 64 * 1024 * 1024;
    bootinfo.memory_map.entries[0].type = MEMORY_MAP_FREE;
    bootinfo.memory_map.num_entries = 1;

    mp_lock();
    stack_set_canary();
    kernel_main(&bootinfo);
    UNREACHABLE();
}

void boot_s_mode(void);
void riscv32_trap_handler(void);
void riscv32_timer_handler(void);

__noreturn void riscv32_setup_m_mode(void) {
    write_medeleg(0xffff);
    write_mideleg(0xffff);

    riscv32_uart_init();

    write_stvec((uint32_t) riscv32_trap_handler);
    write_sie(read_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
    write_sscratch((uint32_t) &cpuvar_fixme);
    write_mscratch((uint32_t) &cpuvar_fixme);
    write_tp((uint32_t) &cpuvar_fixme);

    uint32_t mstatus = read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    mstatus |= 1 << 19;  // FIXME:
    write_mstatus(mstatus);
    write_mepc((uint32_t) boot_s_mode);

    write_satp(0);
    write_pmpaddr0(0xffffffff);
    write_pmpcfg0(0xf);

    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);

    int hart = read_mhartid();
    int interval = 1000000;
    ASSERT(hart < NUM_CPUS_MAX);
    CPUVAR->id = hart;
    CPUVAR->arch.mtimecmp = CLINT_MTIMECMP(hart);
    CPUVAR->arch.mtime = CLINT_MTIME;
    CPUVAR->arch.interval = interval;

    uint32_t *mtimecmp = (uint32_t *) arch_paddr2ptr(CPUVAR->arch.mtimecmp);
    uint32_t *mtime = (uint32_t *) arch_paddr2ptr(CPUVAR->arch.mtime);
    *mtimecmp = *mtime + interval;

    write_mtvec((uint32_t) riscv32_timer_handler);
    write_mstatus(read_mstatus() | MSTATUS_MIE);
    write_mie(read_mie() | MIE_MTIE);

    // TODO: mp init

    asm_mret();
    UNREACHABLE();
}

void arch_idle(void) {
    mp_unlock();
    write_sstatus(read_sstatus() | SSTATUS_SIE);
    asm_wfi();
    mp_lock();
}
