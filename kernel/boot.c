#include <memory.h>
#include <process.h>
#include <thread.h>
#include <ipc.h>
#include <printk.h>
#include <arch.h>

extern char __initfs[];
__asm__(
    ".align 8                \n"
    "__initfs:               \n"
    ".incbin \"initfs.bin\"  \n"
);

static paddr_t initfs_pager(UNUSED struct vmarea *vma, vaddr_t vaddr) {
    void *page = alloc_page();
    if (!page) {
        return 0;
    }

    vaddr_t offset = vaddr - INITFS_BASE;
    vaddr_t copy_from = (vaddr_t) __initfs + offset;
    memcpy(page, PAGE_SIZE, (void *) copy_from, PAGE_SIZE);

    return into_paddr(page);
}

static paddr_t straight_map_pager(UNUSED struct vmarea *vma, vaddr_t vaddr) {
    return vaddr;
}

// Spawns the first user process from the initfs. Here we don't lock objects
// since it won't be problem here: we run this function exactly once on the
// bootstrap processor with interrupts disabled.
static void userland(void) {
    // Create the very first user process and thread.
    struct process *user_process = process_create("memmgr");
    if (!user_process) {
        PANIC("failed to create a process");
    }

    struct thread *thread = thread_create(user_process, INITFS_BASE, 0, 0);
    if (!thread) {
        PANIC("failed to create a user thread");
    }

    // Create a channel connection between the kernel server and the user process.
    struct channel *kernel_ch = channel_create(kernel_process);
    if (!kernel_ch) {
        PANIC("failed to create a channel");
    }

    struct channel *user_ch = channel_create(user_process);
    if (!user_ch) {
        PANIC("failed to create a channel");
    }

    channel_link(kernel_ch, user_ch);

    // Set up pagers.
    int flags = PAGE_WRITABLE | PAGE_USER;
    if (!vmarea_add(user_process, INITFS_BASE, INITFS_IMAGE_SIZE, initfs_pager, NULL, flags)) {
        PANIC("failed to add a vmarea");
    }

    if (!vmarea_add(user_process, STRAIGHT_MAP_BASE, STRAIGHT_MAP_SIZE, straight_map_pager, NULL, flags)) {
        PANIC("failed to add a vmarea");
    }

    // Enqueue the thread into the run queue.
    thread_resume(thread);
}

static void idle(void) {
    while (0) {
        arch_idle();
    }
}

void boot(void) {
    init_boot_stack_canary();

    INFO("Booting Resea...");
    debug_init();
    memory_init();
    arch_init();
    process_init();
    thread_init();

    userland();

    // Perform the very first context switch. The current context will be a
    // CPU-local idle thread.
    DEBUG("first switch...");
    thread_switch();

    // Now we're in the idle thread context.
    idle();
}
