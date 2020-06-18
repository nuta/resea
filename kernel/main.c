#include <config.h>
#include <string.h>
#include "main.h"
#include "kdebug.h"
#include "memory.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

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

// Maps ELF segments in the boot ELF into virtual memory.
static void map_boot_elf(struct bootelf_header *header, struct vm *vm) {
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
            memcpy((void *) vaddr, (void *) paddr, m->num_pages * PAGE_SIZE);
        }
#else
        ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
        ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

        pageattrs_t attrs = PAGE_USER | PAGE_WRITABLE;
        if (m->zeroed) {
            for (size_t j = 0; j < m->num_pages; j++) {
                void *page = kmalloc(PAGE_SIZE);
                ASSERT(page);
                memset(page, 0, PAGE_SIZE);
                vm_link(vm, vaddr, into_paddr(page), attrs);
                vaddr += PAGE_SIZE;
            }
        } else {
            for (size_t j = 0; j < m->num_pages; j++) {
                vm_link(vm, vaddr, paddr, attrs);
                vaddr += PAGE_SIZE;
                paddr += PAGE_SIZE;
            }
        }
#endif
    }
}

/// Initializes the kernel and starts the first task.
NORETURN void kmain(void) {
    printf("\nBooting Resea " VERSION "...\n");
    memory_init();
    task_init();
    mp_start();

    char name[CONFIG_TASK_NAME_LEN];
    struct bootelf_header *bootelf = locate_bootelf_header();
    strncpy(name, (const char *) bootelf->name,
            MIN(sizeof(name), sizeof(bootelf->name)));

    // Create the first userland task.
    struct task *task = task_lookup_unchecked(INIT_TASK_TID);
    ASSERT(task);
    task_create(task, name, bootelf->entry, NULL, 0);
    map_boot_elf(bootelf, &task->vm);

    mpmain();
}

NORETURN void mpmain(void) {
    stack_set_canary();

    // Initialize the idle task for this CPU.
    IDLE_TASK->tid = 0;
    task_create(IDLE_TASK, "(idle)", 0, 0, 0);
    CURRENT = IDLE_TASK;

    // Start context switching and enable interrupts...
    INFO("Booted CPU #%d", mp_self());
    arch_idle();
}
