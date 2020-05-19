#include <elf.h>
#include <config.h>
#include <cstring.h>
#include "main.h"
#include "kdebug.h"
#include "memory.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

extern char __bootelf[];
extern char __bootelf_end[];

// Maps ELF segments in the boot ELF into virtual memory.
static vaddr_t get_boot_elf_entry(void) {
    elf_ehdr_t *ehdr = (elf_ehdr_t *) __bootelf;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        PANIC("invalid boot ELF magic");
    }

    return ehdr->e_entry;
}

// Maps ELF segments in the boot ELF into virtual memory.
static void map_boot_elf(struct vm *vm) {
    elf_ehdr_t *ehdr = (elf_ehdr_t *) __bootelf;
    uint8_t *bootelf_base = (uint8_t *) __bootelf;
    elf_phdr_t *phdrs = (elf_phdr_t *) ((vaddr_t) ehdr + ehdr->e_ehsize);
    for (unsigned i = 0; i < ehdr->e_phnum; i++) {
        vaddr_t vaddr = phdrs[i].p_vaddr;
        if (!vaddr) {
            continue;
        }

        size_t file_off = phdrs[i].p_offset;
        size_t file_size = phdrs[i].p_filesz;
        size_t mem_size = phdrs[i].p_memsz;
        paddr_t paddr = into_paddr(bootelf_base + file_off);
        ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
        ASSERT(IS_ALIGNED(file_off, PAGE_SIZE));
        ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
        ASSERT(mem_size == file_size || file_size == 0);
        ASSERT(IS_ALIGNED(file_size, PAGE_SIZE));
        ASSERT(IS_ALIGNED(mem_size, PAGE_SIZE));

#ifdef NOMMU
        vaddr_t copy_from = (vaddr_t) (bootelf_base + file_off);
        if (file_size > 0 && vaddr != copy_from) {
            DBG("memmove: %p -> %p (%d)", copy_from, vaddr, file_size);
            memmove((void *) vaddr, (void *) copy_from, file_size);
        }
#else
        // Map file contents.
        while (file_size > 0) {
            vm_link(vm, vaddr, paddr, PAGE_USER | PAGE_WRITABLE);
            vaddr += PAGE_SIZE;
            paddr += PAGE_SIZE;
            file_size -= PAGE_SIZE;
        }

        // Map remaining pages (.bss section) with zeroes.
        mem_size -= file_size;
        while (mem_size > 0) {
            void *page = kmalloc(PAGE_SIZE);
            memset(page, 0, PAGE_SIZE);
            vm_link(vm, vaddr, into_paddr(page), PAGE_USER | PAGE_WRITABLE);
            vaddr += PAGE_SIZE;
            paddr += PAGE_SIZE;
            mem_size -= PAGE_SIZE;
        }
#endif
    }
}

/// Initializes the kernel and starts the first task.
void kmain(void) {
    printf("\nBooting Resea " VERSION "...\n");
    memory_init();
    task_init();
    mp_start();

    // Create the first userland task.
    struct task *task = task_lookup(INIT_TASK_TID);
    ASSERT(task);
    task_create(task, "bootstrap", get_boot_elf_entry(), NULL, CAP_ALL);
    map_boot_elf(&task->vm);

    mpmain();
}

void mpmain(void) {
    stack_set_canary();

    // Initialize the idle task for this CPU.
    IDLE_TASK->tid = 0;
    task_create(IDLE_TASK, "(idle)", 0, 0, CAP_ALL);
    CURRENT = IDLE_TASK;

    // Do the very first context switch on this CPU.
    INFO("Booted CPU #%d", mp_self());
    task_switch();

    // We're now in the current CPU's idle task.
    while (true) {
        // Halt the CPU until an interrupt arrives...
        arch_idle();
        // Handled an interrupt. Try switching into a task resumed by an
        // interrupt message.
        task_switch();
    }
}
