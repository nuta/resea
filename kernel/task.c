#include "task.h"

void task_switch(struct task *next) {
    struct task *current = CURRENT_TASK;
    arch_task_switch(current, next);
}
