#include <kernel/main.h>
#include "asm.h"

static void setup_smode(void) {
    kernel_main();
}

void riscv_setup(void) {
  write_satp(0);

  write_medeleg(0xffff);
  write_mideleg(0xffff);
  write_sie(read_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  write_pmpaddr0(0xffffffff);
  write_pmpcfg0(0xf);

  write_mstatus((read_mstatus() & (~MSTATUS_MPP_MASK)) | MSTATUS_MPP_S);
  write_mepc((uint32_t) setup_smode);
  __asm__ __volatile__("mret");
}
