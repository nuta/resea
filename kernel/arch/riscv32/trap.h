#pragma once
#include <types.h>

#define SCAUSE_S_SOFT_INTR      ((1L << 31) | 1)
#define SCAUSE_S_EXT_INTR       ((1L << 31) | 9)
#define SCAUSE_ENV_CALL         8
#define SCAUSE_INST_PAGE_FAULT  12
#define SCAUSE_LOAD_PAGE_FAULT  13
#define SCAUSE_STORE_PAGE_FAULT 15

struct riscv32_trap_frame {
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

void riscv32_handle_trap(struct riscv32_trap_frame *frame);
