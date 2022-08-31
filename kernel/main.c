#include "main.h"
#include "arch.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include <elf.h>
#include <string.h>

void load_boot_elf(struct bootinfo *bootinfo) {
    elf_ehdr_t *header = arch_paddr2ptr(bootinfo->boot_elf);
    if (memcmp(header->e_ident, ELF_MAGIC, 4) != 0) {
        PANIC("bootelf: invalid ELF magic\n");
    }

    task_t tid = task_create("init", header->e_entry, NULL, 0);
    ASSERT(tid > 0);  // TODO: ASSERT_OK
    struct task *task = get_task_by_tid(tid);

    elf_phdr_t *phdrs = (elf_phdr_t *) ((vaddr_t) header + header->e_phoff);
    for (uint16_t i = 0; i < header->e_phnum; i++) {
        elf_phdr_t *phdr = &phdrs[i];
        // Ignore GNU_STACK
        if (!phdr->p_vaddr) {
            continue;
        }

        ASSERT(IS_ALIGNED(phdr->p_filesz, PAGE_SIZE));
        ASSERT(phdr->p_memsz >= phdr->p_filesz);

        arch_vm_map(&task->vm, phdr->p_vaddr,
                    bootinfo->boot_elf + phdr->p_offset, phdr->p_filesz,
                    phdr->p_flags);
        if (phdr->p_memsz > phdr->p_filesz) {
            paddr_t paddr = pm_alloc(phdr->p_memsz - phdr->p_filesz,
                                     PAGE_TYPE_USER(task), PM_ALLOC_ZEROED);
            arch_vm_map(&task->vm, phdr->p_vaddr + phdr->p_filesz, paddr,
                        phdr->p_memsz - phdr->p_filesz, phdr->p_flags);
        }
    }
}

void kernel_main(struct bootinfo *bootinfo) {
    printk("Booting Resea...\n");
    memory_init(bootinfo);
    task_init();

    task_t idle_task = task_create("(idle)", 0, NULL, 0);
    IDLE_TASK = get_task_by_tid(idle_task);
    CURRENT_TASK = IDLE_TASK;

    load_boot_elf(bootinfo);
    task_switch();

    INFO("idle");
    for (;;) {
        //
    }
}
