#include <resea/ipc.h>
#include <resea/task.h>
#include <resea/malloc.h>
#include <elf/elf.h>
#include <string.h>
#include "mm.h"
#include "task.h"
#include "pages.h"
#include "bootfs.h"

extern char __cmdline[];
extern char __zeroed_pages[];
extern char __zeroed_pages_end[];
extern char __free_vaddr_end[];

static paddr_t alloc_pages(struct task *task, vaddr_t vaddr, size_t num_pages) {
    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = vaddr;
    area->paddr = pages_alloc(num_pages);
    area->num_pages = num_pages;
    list_push_back(&task->page_areas, &area->next);
    return area->paddr;
}

/// Allocates a virtual address space by so-called the bump pointer allocation
/// algorithm.
static vaddr_t alloc_virt_pages(struct task *task, size_t num_pages) {
    vaddr_t vaddr = task->free_vaddr;
    size_t size = num_pages * PAGE_SIZE;

    if (vaddr + size >= (vaddr_t) __free_vaddr_end) {
        // Task's virtual memory space has been exhausted.
        task_kill(task);
        return 0;
    }

    task->free_vaddr += size;
    return vaddr;
}

error_t alloc_phy_pages(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                        size_t num_pages) {
    if (*paddr && !is_mappable_paddr(*paddr)) {
        return ERR_INVALID_ARG;
    }

    *vaddr = alloc_virt_pages(task, num_pages);
    if (*paddr) {
        pages_incref(paddr2pfn(*paddr), num_pages);
    } else {
        *paddr = pages_alloc(num_pages);
    }

    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = *vaddr;
    area->paddr = *paddr;
    area->num_pages = num_pages;
    list_push_back(&task->page_areas, &area->next);
    return OK;
}

paddr_t vaddr2paddr(struct task *task, vaddr_t vaddr) {
    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        if (area->vaddr <= vaddr
            && vaddr < area->vaddr + area->num_pages * PAGE_SIZE) {
            return area->paddr + (vaddr - area->vaddr);
        }
    }

    // The page is not mapped. Try filling it with pager.
    return pager(task, vaddr, EXP_PF_USER | EXP_PF_WRITE /* FIXME: strip PF_WRITE */);
}

error_t map_page(task_t tid, vaddr_t vaddr, paddr_t paddr, unsigned flags,
                 bool overwrite) {
    if (overwrite) {
        task_unmap(tid, vaddr);
    }

    while (true) {
        paddr_t kpage = pages_alloc(1);
        error_t err = task_map(tid, vaddr, paddr, kpage, flags);
        switch (err) {
            case ERR_TRY_AGAIN:
                continue;
            default:
                return err;
        }
    }
}

paddr_t pager(struct task *task, vaddr_t vaddr, unsigned fault) {
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

    if (fault & EXP_PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (perhaps segfault?)", task->name,
             vaddr);
        return 0;
    }

    // The `cmdline` for main().
    if (vaddr == (vaddr_t) __cmdline) {
        paddr_t paddr = alloc_pages(task, vaddr, 1);
        ASSERT_OK(map_page(INIT_TASK, paddr, paddr, MAP_W, false));
        memset((void *) paddr, 0, PAGE_SIZE);
        strncpy((void *) paddr, task->cmdline, PAGE_SIZE);
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
        paddr_t paddr = alloc_pages(task, vaddr, 1);
        ASSERT_OK(map_page(INIT_TASK, paddr, paddr, MAP_W, false));
        memset((void *) paddr, 0, PAGE_SIZE);
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
            paddr_t paddr = alloc_pages(task, vaddr, 1);
            ASSERT_OK(map_page(INIT_TASK, paddr, paddr, MAP_W, false));
            size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
            read_file(task->file, offset_in_segment, (void *) paddr, PAGE_SIZE);
            return paddr;
        }
    }

    WARN("invalid memory access (addr=%p), killing %s...", vaddr, task->name);
    return 0;
}
