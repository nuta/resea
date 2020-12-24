#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <config.h>
#include <print_macros.h>
#include <types.h>

/// The kernel log (ring) buffer.
struct klog {
    char buf[CONFIG_KLOG_BUF_SIZE];
    size_t head;
    size_t tail;
};

void klog_write(char ch);
size_t klog_read(char *buf, size_t buf_len);
void printk(const char *fmt, ...);

// Implemented in arch.
void arch_printchar(char ch);

#endif
