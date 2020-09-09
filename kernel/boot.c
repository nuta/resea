#include <config.h>
#include <string.h>
#include "boot.h"
#include "kdebug.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

// Defined in arch.
extern uint8_t __bootelf[];
extern uint8_t __bootelf_end[];

static struct bootelf_header *locate_bootelf_header(void) {
    const offset_t offsets[] = {
        0x1000,  // x64
        0x10000, // arm64
    };

    for (size_t i = 0; i < sizeof(offsets) / sizeof(*offsets); i++) {
        struct bootelf_header *header =
            (struct bootelf_header *) &__bootelf[offsets[i]];
        if (!memcmp(header, BOOTELF_MAGIC, sizeof(header->magic))) {
            return header;
        }
    }

    PANIC("failed to locate the boot ELF header");
}

#if !defined(CONFIG_NOMMU)
/// Allocates a memory page for the first user task.
static void *alloc_page(void) {
    static uint8_t heap[CONFIG_ONESHOT_HEAP_SIZE * PAGE_SIZE] __aligned(PAGE_SIZE);
    static uint8_t *current = heap;
    if (current >= heap + sizeof(heap)) {
        PANIC("run out of memory for init task "
            "(hint: increase ONESHOT_HEAP_SIZE in the build config)");
    }

    void *ptr = current;
    current += PAGE_SIZE;
    return ptr;
}

static error_t map_page(struct task *task, vaddr_t vaddr, paddr_t paddr,
                        unsigned flags) {
    while (true) {
        paddr_t kpage = into_paddr(alloc_page());
        error_t err = task_map_page(task, vaddr, paddr, kpage, MAP_W);
        if (err != ERR_TRY_AGAIN) {
            return err;
        }
    }
}
#endif

// Maps ELF segments in the boot ELF into virtual memory.
static void map_bootelf(struct bootelf_header *header, struct task *task) {
    TRACE("boot ELF: entry=%p", header->entry);
    for (unsigned i = 0; i < header->num_mappings; i++) {
        struct bootelf_mapping *m = &header->mappings[i];
        vaddr_t vaddr = m->vaddr;
        paddr_t paddr = into_paddr(&__bootelf[m->offset]);

        TRACE("boot ELF: %p -> %p (%dKiB%s)",
              vaddr,
              (m->zeroed) ? 0 : paddr,
              m->num_pages * PAGE_SIZE / 1024,
              (m->zeroed) ? ", zeroed" : "");

#ifdef CONFIG_NOMMU
        if (m->zeroed) {
            memset((void *) vaddr, 0, m->num_pages * PAGE_SIZE);
        } else {
            if (vaddr == paddr) {
                continue;
            }
            memcpy((void *) vaddr, (void *) paddr, m->num_pages * PAGE_SIZE);
        }
#else
        ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
        ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

        if (m->zeroed) {
            for (size_t j = 0; j < m->num_pages; j++) {
                void *page = alloc_page();
                ASSERT(page);
                memset(page, 0, PAGE_SIZE);
                error_t err = map_page(task, vaddr, into_paddr(page), MAP_W);
                ASSERT_OK(err);
                vaddr += PAGE_SIZE;
            }
        } else {
            for (size_t j = 0; j < m->num_pages; j++) {
                error_t err = map_page(task, vaddr, paddr, MAP_W);
                ASSERT_OK(err);
                vaddr += PAGE_SIZE;
                paddr += PAGE_SIZE;
            }
        }
#endif
    }
}

/// Initializes the kernel and starts the first task.
__noreturn void kmain(void) {
    printf("\nBooting Resea " VERSION "...\n");
    task_init();
    mp_start();

    char name[CONFIG_TASK_NAME_LEN];
    struct bootelf_header *bootelf = locate_bootelf_header();
    strncpy(name, (const char *) bootelf->name,
            MIN(sizeof(name), sizeof(bootelf->name)));

    // Create the first userland task.
    struct task *task = task_lookup_unchecked(INIT_TASK);
    ASSERT(task);
    error_t err = task_create(task, name, bootelf->entry, NULL, TASK_ALL_CAPS);
    ASSERT_OK(err);
    map_bootelf(bootelf, task);

    mpmain();
}

__noreturn void mpmain(void) {
    stack_set_canary();

    // Initialize the idle task for this CPU.
    IDLE_TASK->tid = 0;
    error_t err = task_create(IDLE_TASK, "(idle)", 0, NULL, 0);
    ASSERT_OK(err);
    CURRENT = IDLE_TASK;

    // Start context switching and enable interrupts...
    INFO("Booted CPU #%d", mp_self());
    arch_idle();
}
