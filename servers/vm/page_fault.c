#include "page_fault.h"
#include "bootfs.h"
#include "page_alloc.h"
#include "task.h"
#include <elf/elf.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/task.h>
#include <string.h>

extern char __cmdline[];
extern char __zeroed_pages[];
extern char __zeroed_pages_end[];

static vaddr_t tmp_page = 0;

error_t map_page(struct task *task, vaddr_t vaddr, paddr_t paddr,
                 unsigned flags, bool overwrite) {
    if (overwrite) {
        vm_unmap(task->tid, vaddr);
    }

    // Calls vm_map multiple times because the kernel needs multiple memory
    // pages (kpage) for multi-level page table structures.
    while (true) {
        paddr_t kpage = 0;
        error_t err = task_page_alloc(task, NULL, &kpage, 1);
        if (err != OK) {
            return err;
        }

        err = vm_map(task->tid, vaddr, paddr, kpage, flags);
        switch (err) {
            case ERR_TRY_AGAIN:
                continue;
            default:
                if (err != OK) {
                    WARN_DBG(
                        "%s: failed to map a page: %s (paddr=%p, vaddr=%p, kpage=%p)",
                        task->name, err2str(err), paddr, vaddr, kpage);
                }
                task_page_free(task, kpage);
                return err;
        }
    }
}

paddr_t handle_page_fault(struct task *task, vaddr_t vaddr, vaddr_t ip,
                          unsigned fault) {
    if (vaddr < PAGE_SIZE) {
        WARN("%s (%d): null pointer dereference at vaddr=%p, ip=%p", task->name,
             task->tid, vaddr, ip);
        return 0;
    }

    vaddr_t vaddr_original = vaddr;
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

    if (fault & EXP_PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (IP=%p, perhaps segfault?)",
             task->name, vaddr_original, ip);
        return 0;
    }

    // The `cmdline` for main().
    if (vaddr == (vaddr_t) __cmdline) {
        paddr_t paddr = 0;
        // FIXME: Don't ues ASSERT_OK to prevent vm from crashing.
        ASSERT_OK(task_page_alloc(task, &vaddr, &paddr, 1));
        ASSERT_OK(
            map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
        memset((void *) tmp_page, 0, PAGE_SIZE);
        strncpy2((void *) tmp_page, task->cmdline, PAGE_SIZE);
        return paddr;
    }

    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        if (area->vaddr <= vaddr
            && vaddr < area->vaddr + area->num_pages * PAGE_SIZE) {
            return area->paddr + (vaddr - area->vaddr);
        }
    }

    // Zeroed pages.
    vaddr_t zeroed_pages_start = (vaddr_t) __zeroed_pages;
    vaddr_t zeroed_pages_end = (vaddr_t) __zeroed_pages_end;
    if (zeroed_pages_start <= vaddr && vaddr < zeroed_pages_end) {
        // The accessed page is zeroed one (.bss section, stack, or heap).
        // FIXME: Don't ues ASSERT_OK to prevent vm from crashing.
        paddr_t paddr = 0;
        ASSERT_OK(task_page_alloc(task, &vaddr, &paddr, 1));
        ASSERT_OK(
            map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
        memset((void *) tmp_page, 0, PAGE_SIZE);
        return paddr;
    }

    // Look for the associated program header.
    struct elf64_phdr *phdr = NULL;
    if (task->ehdr) {
        for (unsigned i = 0; i < task->ehdr->e_phnum; i++) {
            // Ignore GNU_STACK
            if (!task->phdrs[i].p_vaddr) {
                continue;
            }

            vaddr_t start = task->phdrs[i].p_vaddr;
            vaddr_t end = start + task->phdrs[i].p_memsz;
            if (start <= vaddr && vaddr <= end) {
                phdr = &task->phdrs[i];
                break;
            }
        }

        if (phdr) {
            // Allocate a page and fill it with the file data.
            // FIXME: Don't ues ASSERT_OK to prevent vm from crashing.
            paddr_t paddr = 0;
            ASSERT_OK(task_page_alloc(task, &vaddr, &paddr, 1));
            ASSERT_OK(
                map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
            size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
            read_file(task->file, offset_in_segment, (void *) tmp_page,
                      PAGE_SIZE);
            return paddr;
        }
    }

    WARN("invalid memory access (addr=%p), killing %s...", vaddr_original,
         task->name);
    return 0;
}

void page_fault_init(void) {
    tmp_page = virt_page_alloc(vm_task, 1);
}
