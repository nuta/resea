#include "switch.h"
#include <kernel/arch.h>
#include <kernel/task.h>

void arch_task_switch(struct task *prev, struct task *next) {
    riscv_task_switch(&prev->arch.sp, &next->arch.sp);
}
