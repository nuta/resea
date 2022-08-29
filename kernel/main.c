#include "main.h"
#include "arch.h"
#include "printk.h"

void kernel_main(void) {
    printk("Booting Resea...\n");
    int a = 0;
    int b = 1;
    int c = b / a;
    printk("c = %d\n", c);
}
