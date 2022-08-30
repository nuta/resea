#include "main.h"
#include "arch.h"
#include "memory.h"
#include "printk.h"

void kernel_main(struct bootinfo *bootinfo) {
    printk("Booting Resea...\n");
    memory_init(bootinfo);
}
