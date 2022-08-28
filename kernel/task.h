#ifndef __TASK_H__
#define __TASK_H__

#include <types.h>
#include <arch_types.h>

struct task {
    /// The arch-specific fields.
    struct arch_task arch;
    task_t tid;

    struct task *pager;
    uint16_t ref_count;
};

#endif
