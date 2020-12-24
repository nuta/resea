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
        *((volatile uint32_t *) from_paddr(paddr)) = (uint32_t) boot;

        __asm__ __volatile__("sev");
    }
}
