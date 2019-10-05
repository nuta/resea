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

static void userland(struct init_args *args);

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
#ifdef CONFIG_MP
    arch_mp_init();
#endif

    thread_first_switch();
    while (1) {
        arch_idle();
    }
}

#ifdef CONFIG_MP
/// The entry point for application processors (processors other than the CPU
/// which booted the system).
void boot_ap(void) {
    thread_first_switch();
    while (1) {
        arch_idle();
    }
}
#endif

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
    init_process = process_create("memmgr");
    ASSERT(init_process);

    struct thread *thread = thread_create(init_process, INITFS_ADDR,
        0 /* stack */, THREAD_INFO_ADDR, 0 /* arg */);
    ASSERT(thread);

    // Create a channel connection between the kernel server and the user
    // process.
    struct channel *kernel_ch = channel_create(kernel_process);
    ASSERT(kernel_ch);
    struct channel *user_ch = channel_create(init_process);
    ASSERT(kernel_ch);

    channel_transfer(kernel_ch, kernel_server_ch);
    channel_link(kernel_ch, user_ch);

    // Set up pagers.
    int flags = PAGE_WRITABLE | PAGE_USER;
    error_t err;
    err = vmarea_create(init_process, INITFS_ADDR, INITFS_END, initfs_pager, NULL,
                        flags);
    ASSERT(err == OK);
    err = vmarea_create(init_process, STRAIGHT_MAP_ADDR, STRAIGHT_MAP_END,
                        straight_map_pager, NULL, flags);
    ASSERT(err == OK);

    // Fill init_args.
    inlined_memcpy(__initfs + INIT_ARGS_OFFSET, args, sizeof(*args));

    // Enqueue the thread into the run queue.
    thread_resume(thread);
}
