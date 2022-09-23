#pragma once
#include <print.h>

#define KLOG_BUF_SIZE 1024

/// The kernel log (ring) buffer.
struct klog {
    char buf[KLOG_BUF_SIZE];
    size_t head;
    size_t tail;
};

void handle_console_interrupt(void);
int console_read(char *buf, int max_len);
void klog_write(char ch);
size_t klog_read(char *buf, size_t buf_len);
