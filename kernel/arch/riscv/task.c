#include <arch.h>
#include <kernel/task.h>

void riscv_switch(uint32_t *prev_sp, uint32_t *next_sp);

void arch_task_switch(struct task *prev, struct task *next) {
    riscv_task_switch(prev->arch.sp, next->arch.sp);
}
