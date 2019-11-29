#ifndef __STATS_H__
#define __STATS_H__

#include <types.h>

// TODO: MP support (these should be atomic operations)
#define INC_STAT(name) kernel_stats.name++
#define READ_STAT(name) kernel_stats.name

struct kernel_stats {
    size_t ipc_total;
    size_t page_fault_total;
    size_t context_switch_total;
    size_t kernel_call_total;
};

extern struct kernel_stats kernel_stats;

#endif
