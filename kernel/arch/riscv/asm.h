#pragma once
#include <types.h>

#define MSTATUS_MPP_U    (0b00 << 11)
#define MSTATUS_MPP_S    (0b01 << 11)
#define MSTATUS_MPP_M    (0b11 << 11)
#define MSTATUS_MPP_MASK (0b11 << 11)

#define SSTATUS_SPP_MASK (1 << 8)
#define SSTATUS_SPP_MASK (1 << 8)

#define SIE_SSIE (1 << 1)
#define SIE_STIE (1 << 5)
#define SIE_SEIE (1 << 9)

//  TODO: Support RV64 (XLEN > 32)

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

static inline uint32_t read_sie(void) {
    uint32_t value;
    __asm__ __volatile__("csrr %0, sie" : "=r"(value));
    return value;
}

static inline void write_sie(uint32_t value) {
    __asm__ __volatile__("csrw sie, %0" ::"r"(value));
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

static inline void write_satp(uint32_t value) {
    __asm__ __volatile__("csrw satp, %0" ::"r"(value));
}

static inline void write_tp(uint32_t value) {
    __asm__ __volatile__("mv tp, %0" ::"r"(value));
}
