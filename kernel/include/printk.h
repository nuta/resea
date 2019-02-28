#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <arch.h>
#include <debug.h>

void printk(const char *fmt, ...);

#define DEBUG(fmt, ...) printk("DEBUG: " fmt "\n", ## __VA_ARGS__)
#define DEBUG_USER(fmt, ...) \
    printk("#%s.%d: " fmt "\n", CURRENT->process->name, CURRENT->tid, ## __VA_ARGS__)
#define INFO(fmt, ...) printk(fmt "\n", ## __VA_ARGS__)
#define WARN(fmt, ...) printk("WARN: " fmt "\n", ## __VA_ARGS__)
#define BUG(fmt, ...) do {\
        printk("[PANIC] BUG: " fmt "\n", ## __VA_ARGS__); \
        backtrace(); \
        arch_panic(); \
    } while(0)

#define PANIC(fmt, ...) do { \
        printk("[PANIC] " fmt "\n", ## __VA_ARGS__); \
        backtrace(); \
        arch_panic(); \
    } while(0)

#endif
