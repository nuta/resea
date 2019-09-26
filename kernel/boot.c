#include <arch.h>
#include <init_args.h>
#include <debug.h>
#include <channel.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <server.h>
#include <thread.h>
#include <timer.h>

extern char __initfs[];

/// The pager which maps the initfs image in the kernel executable image.
static paddr_t initfs_pager(UNUSED struct vmarea *vma, vaddr_t vaddr) {
    ASSERT(((vaddr_t) &__initfs & (PAGE_SIZE - 1)) == 0 &&
           "initfs is not aligned");

    return into_paddr(__initfs + (vaddr - INITFS_ADDR));
}

/// The pager which maps the physical page whose address is identical with the
/// requested virtual address.
static paddr_t straight_map_pager(UNUSED struct vmarea *vma, vaddr_t vaddr) {
    return vaddr;
}

// Spawns the first user process from the initfs.
static void userland(struct init_args *args) {
    // Create the very first user process and thread.
    struct process *user_process = process_create("memmgr");
    if (!user_process) {
        PANIC("failed to create a process");
    }

    struct thread *thread = thread_create(user_process, INITFS_ADDR,
        0 /* stack */, THREAD_INFO_ADDR, 0 /* arg */);
    if (!thread) {
        PANIC("failed to create a user thread");
    }

    // Create a channel connection between the kernel server and the user
    // process.
    struct channel *kernel_ch = channel_create(kernel_process);
    if (!kernel_ch) {
        PANIC("failed to create a channel");
    }

    struct channel *user_ch = channel_create(user_process);
    if (!user_ch) {
        PANIC("failed to create a channel");
    }

    channel_transfer(kernel_ch, kernel_server_ch);
    channel_link(kernel_ch, user_ch);

    // Set up pagers.
    int flags = PAGE_WRITABLE | PAGE_USER;
    if (vmarea_add(user_process, INITFS_ADDR, INITFS_END, initfs_pager, NULL,
                   flags) != OK) {
        PANIC("failed to add a vmarea");
    }

    if (vmarea_add(user_process, STRAIGHT_MAP_ADDR, STRAIGHT_MAP_END,
                   straight_map_pager, NULL, flags) != OK) {
        PANIC("failed to add a vmarea");
    }

    // Fill init_args.
    inlined_memcpy(__initfs + INIT_ARGS_OFFSET, args, sizeof(*args));

    // Enqueue the thread into the run queue.
    thread_resume(thread);
}

/// Initializes the kernel and starts the first userland process.
void boot(void) {
    struct init_args init_args;
    init_boot_stack_canary();

    INFO("Booting Resea...");
    debug_init();
    memory_init();
    arch_init(&init_args);
    timer_init();
    process_init();
    thread_init();
    kernel_server_init();

    userland(&init_args);

    // Perform the very first context switch. The current context will become a
    // CPU-local idle thread.
    thread_switch();

    // Now we're in the CPU-local idle thread context.
    while (1) {
        arch_idle();
    }
}
