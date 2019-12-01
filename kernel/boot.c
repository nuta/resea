#include <arch.h>
#include <bootinfo.h>
#include <channel.h>
#include <memory.h>
#include <support/printk.h>
#include <process.h>
#include <server.h>
#include <thread.h>
#include <timer.h>
#include <support/stack_protector.h>
#include <support/kasan.h>

static void userland(struct bootinfo *args);

/// Initializes the kernel and starts the first userland process.
void boot(void) {
    struct bootinfo bootinfo;
    init_boot_stack_canary();
#ifdef DEBUG_BUILD
    kasan_init();
#endif

    INFO("Booting Resea... (version " VERSION ")");
    memory_init();
    arch_init(&bootinfo);
    timer_init();
    process_init();
    thread_init();
    kernel_server_init();
    userland(&bootinfo);
    arch_mp_init();

    thread_first_switch();
    while (1) {
        arch_idle();
    }
}

/// The entry point for application processors (processors other than the CPU
/// which booted the system).
void boot_ap(void) {
    init_boot_stack_canary();
    INFO("successfully booted CPU #%d", arch_get_cpu_id());

    // TODO: Fix locking bugs and enable threading!

    while (1) {
        arch_halt();
    }
}

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
static void userland(struct bootinfo *args) {
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

    // Fill bootinfo.
    inlined_memcpy(__initfs + BOOTINFO_OFFSET, args, sizeof(*args));

    // Enqueue the thread into the run queue.
    thread_resume(thread);
}
