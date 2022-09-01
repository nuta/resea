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

void riscv32_trap_handler(void);
void riscv32_trap(struct risc32_trap_frame *frame) {
    uint32_t tp;
    __asm__ __volatile__("mv %0, tp" : "=r"(tp));
    uint32_t sscratch;
    __asm__ __volatile__("csrr %0, sscratch" : "=r"(sscratch));

    uint32_t scause = read_scause();
    if (scause == 8) {
        frame->sepc += 4;
    }

    TRACE("trap sepc=%p, scause=%p, tp=%p, sscratch=%pm next_sepc=%p",
          read_sepc(), read_scause(), tp, sscratch, frame->sepc);
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
