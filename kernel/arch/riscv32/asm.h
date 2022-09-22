#pragma once
#include <kernel/arch.h>

#define MSTATUS_MIE      (1 << 3)
#define MSTATUS_MPP_U    (0b00 << 11)
#define MSTATUS_MPP_S    (0b01 << 11)
#define MSTATUS_MPP_M    (0b11 << 11)
#define MSTATUS_MPP_MASK (0b11 << 11)

#define MIE_MTIE (1 << 7)

#define SSTATUS_SPP (1 << 8)
#define SSTATUS_SIE (1 << 1)
#define SSTATUS_SUM (1 << 18)

#define SIE_SSIE (1 << 1)
#define SIE_STIE (1 << 5)
#define SIE_SEIE (1 << 9)

#define CLINT_PADDR 0x2000000
#define CLINT_MTIME (CLINT_PADDR + 0xbff8)
#define CLINT_MTIMECMP(hartid)                                                 \
    (CLINT_PADDR + 0x4000 + sizeof(uint64_t) * (hartid))

static inline void write_mepc(uint32_t value) {
    __asm__ __volatile__("csrw mepc, %0" ::"r"(value));
}

static inline uint32_t read_mstatus(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline void write_mstatus(uint32_t value) {
    __asm__ __volatile__("csrw mstatus, %0" ::"r"(value));
}

static inline uint32_t read_mhartid(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mhartid" : "=r"(value));
    return value;
}

static inline uint32_t read_mie(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mie" : "=r"(value));
    return value;
}

static inline void write_mie(uint32_t value) {
    __asm__ __volatile__("csrw mie, %0" ::"r"(value));
}

static inline void write_mtvec(uint32_t value) {
    __asm__ __volatile__("csrw mtvec, %0" ::"r"(value));
}

static inline void write_mscratch(uint32_t value) {
    __asm__ __volatile__("csrw mscratch, %0" ::"r"(value));
}

static inline uint32_t read_sie(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sie" : "=r"(value));
    return value;
}

static inline void write_sie(uint32_t value) {
    __asm__ __volatile__("csrw sie, %0" ::"r"(value));
}

static inline uint32_t read_sip(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sip" : "=r"(value));
    return value;
}

static inline void write_sip(uint32_t value) {
    __asm__ __volatile__("csrw sip, %0" ::"r"(value));
}

static inline uint32_t read_medeleg(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, medeleg" : "=r"(value));
    return value;
}

static inline uint32_t read_mideleg(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, mideleg" : "=r"(value));
    return value;
}

static inline void write_medeleg(uint32_t value) {
    __asm__ __volatile__("csrw medeleg, %0" ::"r"(value));
}

static inline void write_mideleg(uint32_t value) {
    __asm__ __volatile__("csrw mideleg, %0" ::"r"(value));
}

static inline void write_pmpaddr0(uint32_t value) {
    __asm__ __volatile__("csrw pmpaddr0, %0" ::"r"(value));
}

static inline void write_pmpcfg0(uint32_t value) {
    __asm__ __volatile__("csrw pmpcfg0, %0" ::"r"(value));
}

static inline void write_stvec(uint32_t value) {
    __asm__ __volatile__("csrw stvec, %0" ::"r"(value));
}

static inline uint32_t read_sepc(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sepc" : "=r"(value));
    return value;
}

static inline void write_sepc(uint32_t value) {
    __asm__ __volatile__("csrw sepc, %0" ::"r"(value));
}

static inline uint32_t read_sstatus(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sstatus" : "=r"(value));
    return value;
}

static inline void write_sstatus(uint32_t value) {
    __asm__ __volatile__("csrw sstatus, %0" ::"r"(value));
}

static inline uint32_t read_scause(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, scause" : "=r"(value));
    return value;
}

static inline uint32_t read_stval(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, stval" : "=r"(value));
    return value;
}

static inline uint32_t read_satp(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, satp" : "=r"(value));
    return value;
}

static inline void write_satp(uint32_t value) {
    __asm__ __volatile__("csrw satp, %0" ::"r"(value));
}

static inline void write_sscratch(uint32_t value) {
    __asm__ __volatile__("csrw sscratch, %0" ::"r"(value));
}

static inline void write_tp(uint32_t value) {
    __asm__ __volatile__("mv tp, %0" ::"r"(value));
}

static inline void asm_sfence_vma(void) {
    __asm__ __volatile__("sfence.vma zero, zero" ::: "memory");
}

static inline void asm_mret(void) {
    __asm__ __volatile__("mret");
}

static inline void asm_wfi(void) {
    __asm__ __volatile__("wfi");
}

static inline void mmio_write8(paddr_t reg, uint8_t val) {
    *((volatile uint8_t *) arch_paddr2ptr(reg)) = val;
}

static inline void mmio_write32(paddr_t reg, uint32_t val) {
    *((volatile uint32_t *) arch_paddr2ptr(reg)) = val;
}

static inline uint8_t mmio_read8(paddr_t reg) {
    return *((volatile uint8_t *) arch_paddr2ptr(reg));
}

static inline uint32_t mmio_read32(paddr_t reg) {
    return *((volatile uint32_t *) arch_paddr2ptr(reg));
}
