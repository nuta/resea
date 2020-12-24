#include "boot.h"
#include "kdebug.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"
#include <bootinfo.h>
#include <config.h>
#include <string.h>

// Defined in arch.
extern uint8_t __bootelf[];
extern uint8_t __bootelf_end[];

static struct bootelf_header *locate_bootelf_header(void) {
    const offset_t offsets[] = {
        0x1000,   // x64
        0x10000,  // arm64
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
static void *alloc_page(struct bootinfo *bootinfo) {
    for (int i = 0; i < NUM_BOOTINFO_MEMMAP_MAX; i++) {
        struct bootinfo_memmap_entry *m = &bootinfo->memmap[i];
        if (m->type != BOOTINFO_MEMMAP_TYPE_AVAILABLE) {
            continue;
        }

        if (m->len >= PAGE_SIZE) {
            ASSERT(IS_ALIGNED(m->base, PAGE_SIZE));
            void *ptr = from_paddr(m->base);
            m->base += PAGE_SIZE;
            m->len -= PAGE_SIZE;
            return ptr;
        }
    }

    PANIC("run out of memory for the initial task's memory space");
}

static error_t map_page(struct bootinfo *bootinfo, struct task *task,
                        vaddr_t vaddr, paddr_t paddr, unsigned flags) {
    static paddr_t unused_kpage = 0;
    while (true) {
        paddr_t kpage =
            unused_kpage ? unused_kpage : into_paddr(alloc_page(bootinfo));
        error_t err = vm_map(task, vaddr, paddr, kpage, MAP_TYPE_READWRITE);
        // TODO: Free the unused `kpage`.
        if (err == ERR_TRY_AGAIN) {
            unused_kpage = 0;
            continue;
        }

        unused_kpage = kpage;
        return err;
    }
}
#endif

// Maps ELF segments in the boot ELF into virtual memory.
static void map_bootelf(struct bootinfo *bootinfo,
                        struct bootelf_header *header, struct task *task) {
    TRACE("boot ELF: entry=%p", header->entry);
    for (unsigned i = 0; i < BOOTELF_NUM_MAPPINGS_MAX; i++) {
        struct bootelf_mapping *m = &header->mappings[i];
        vaddr_t vaddr = m->vaddr;
        paddr_t paddr = into_paddr(&__bootelf[m->offset]);

        if (!m->vaddr) {
            continue;
        }

        TRACE("boot ELF: %p -> %p (%dKiB%s)", vaddr, (m->zeroed) ? 0 : paddr,
              m->num_pages * PAGE_SIZE / 1024, (m->zeroed) ? ", zeroed" : "");

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
                void *page = alloc_page(bootinfo);
                ASSERT(page);
                memset(page, 0, PAGE_SIZE);
                error_t err = map_page(bootinfo, task, vaddr, into_paddr(page),
                                       MAP_TYPE_READWRITE);
                ASSERT_OK(err);
                vaddr += PAGE_SIZE;
            }
        } else {
            for (size_t j = 0; j < m->num_pages; j++) {
                error_t err =
                    map_page(bootinfo, task, vaddr, paddr, MAP_TYPE_READWRITE);
                ASSERT_OK(err);
                vaddr += PAGE_SIZE;
                paddr += PAGE_SIZE;
            }
        }
#endif
    }
}

/// Initializes the kernel and starts the first task.
__noreturn void kmain(struct bootinfo *bootinfo) {
    printf("\nBooting Resea " VERSION "...\n");
    task_init();
    mp_start();

    // Look for the boot elf header.
    char name[CONFIG_TASK_NAME_LEN];
    struct bootelf_header *bootelf = locate_bootelf_header();
    strncpy(name, (const char *) bootelf->name,
            MIN(sizeof(name), sizeof(bootelf->name)));

    // Copy the bootinfo struct to the boot elf header.
#ifndef CONFIG_NOMMU
    // FIXME: bootelf->bootinfo could exist in ROM.
    memcpy(&bootelf->bootinfo, bootinfo, sizeof(*bootinfo));
#endif

    // Create the first userland task.
    struct task *task = task_lookup_unchecked(INIT_TASK);
    ASSERT(task);
    error_t err = task_create(task, name, bootelf->entry, NULL, TASK_ALL_CAPS);
    ASSERT_OK(err);
    map_bootelf(bootinfo, bootelf, task);

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
    if (!mp_is_bsp()) {
        PANIC("TODO: Support context switching in SMP");
    }
    arch_idle();
}
