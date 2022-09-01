#include "asm.h"
#include "vm.h"
#include <kernel/arch.h>
#include <kernel/main.h>
#include <kernel/printk.h>

// FIXME:
struct cpuvar cpuvar_fixme;

__noreturn static void setup_smode(void) {
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

void riscv32_trap_handler(void);
void riscv32_trap(void) {
    uint32_t tp;
    __asm__ __volatile__("mv %0, tp" : "=r"(tp));
    uint32_t sscratch;
    __asm__ __volatile__("csrr %0, sscratch" : "=r"(sscratch));
    TRACE("trap sepc=%p, scause=%p, tp=%p, sscratch=%p", read_sepc(),
          read_scause(), tp, sscratch);
}

__noreturn void riscv32_setup(void) {
    write_medeleg(0xffff);
    write_mideleg(0xffff);
    // TODO:
    //   write_sie(read_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

    write_stvec((uint32_t) riscv32_trap_handler);
    write_sscratch((uint32_t) &cpuvar_fixme);
    write_tp((uint32_t) &cpuvar_fixme);

    write_satp(0);
    write_pmpaddr0(0xffffffff);
    write_pmpcfg0(0xf);

    uint32_t mstatus = read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    mstatus |= MSTATUS_SUM;
    mstatus |= 1 << 19;  // FIXME:
    write_mstatus(mstatus);

    write_mepc((uint32_t) setup_smode);
    __asm__ __volatile__("mret");
    UNREACHABLE();
}
