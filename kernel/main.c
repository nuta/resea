#include "main.h"
#include "arch.h"
#include "memory.h"
#include "printk.h"
#include "task.h"

void kernel_main(struct bootinfo *bootinfo) {
    printk("Booting Resea...\n");
    memory_init(bootinfo);
    task_init();
}
