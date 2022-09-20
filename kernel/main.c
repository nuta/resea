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
    ASSERT_OK(tid);
    struct task *task = get_task_by_tid(tid);

    elf_phdr_t *phdrs = (elf_phdr_t *) ((vaddr_t) header + header->e_phoff);
    for (uint16_t i = 0; i < header->e_phnum; i++) {
        elf_phdr_t *phdr = &phdrs[i];
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        ASSERT(phdr->p_memsz >= phdr->p_filesz);

        char r = (phdr->p_flags & PF_R) ? 'r' : '-';
        char w = (phdr->p_flags & PF_W) ? 'w' : '-';
        char x = (phdr->p_flags & PF_X) ? 'x' : '-';
        size_t size_in_kb = phdr->p_memsz / 1024;
        TRACE("bootelf: %p - %p %c%c%c (%d KiB)", phdr->p_vaddr,
              phdr->p_vaddr + phdr->p_memsz, r, w, x, size_in_kb);

        paddr_t paddr =
            pm_alloc(phdr->p_memsz, PAGE_TYPE_USER(task), PM_ALLOC_ZEROED);
        memcpy(arch_paddr2ptr(paddr),
               (void *) ((vaddr_t) header + phdr->p_offset), phdr->p_filesz);

        unsigned attrs = PAGE_USER;
        attrs |= (phdr->p_flags & PF_R) ? PAGE_READABLE : 0;
        attrs |= (phdr->p_flags & PF_W) ? PAGE_WRITABLE : 0;
        attrs |= (phdr->p_flags & PF_X) ? PAGE_EXECUTABLE : 0;

        error_t err = arch_vm_map(&task->vm, phdr->p_vaddr, paddr,
                                  ALIGN_UP(phdr->p_memsz, PAGE_SIZE), attrs);
        if (err != OK) {
            PANIC("bootelf: failed to map %p - %p", phdr->p_vaddr,
                  phdr->p_vaddr + phdr->p_memsz);
        }
    }

    task_resume(task);
}

void kernel_main(struct bootinfo *bootinfo) {
    printk("Booting Resea...\n");
    memory_init(bootinfo);
    task_init();
    task_mp_init();

    load_boot_elf(bootinfo);
    task_switch();

    for (;;) {
        arch_idle();
        task_switch();
    }
}
