#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <print_macros.h>
#include <types.h>

#define KLOG_BUF_SIZE 4096

/// The kernel log (ring) buffer.
struct klog {
    char buf[KLOG_BUF_SIZE];
    size_t head;
    size_t tail;
};

void klog_write(char ch);
size_t klog_read(char *buf, size_t buf_len);
struct task;
void klog_listen(struct task *task);
void klog_unlisten(struct task *task);
void printk(const char *fmt, ...);

// Implemented in arch.
void arch_printchar(char ch);

#endif
