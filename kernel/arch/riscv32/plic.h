#pragma once

#define PLIC_ADDR ((paddr_t) 0x0c000000)
#define PLIC_SIZE 0x400000

#define PLIC_SENABLE(hart)   (PLIC_ADDR + 0x2080 + (hart) *0x100)
#define PLIC_SPRIORITY(hart) (PLIC_ADDR + 0x201000 + (hart) *0x2000)
#define PLIC_SCLAIM(hart)    (PLIC_ADDR + 0x201004 + (hart) *0x2000)

int plic_pending(void);
void plic_ack(int irq);
void riscv32_plic_init(void);
