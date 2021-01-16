#include <machine/machine.h>
#include <machine/peripherals.h>
#include <printk.h>

#define NUM_CPUS 4

extern char boot[];

void machine_mp_start(void) {
    for (int cpu = 1; cpu < NUM_CPUS; cpu++) {
        TRACE("Booting CPU #%d...", cpu);

        // https://leiradel.github.io/2019/01/20/Raspberry-Pi-Stubs.html
        paddr_t paddr = 0xd8 + cpu * 8;
        *((volatile uint32_t *) paddr2ptr(paddr)) = (uint64_t) boot & 0xffffffff;

        __asm__ __volatile__("sev");
    }
}
