#include <resea/ipc.h>
#include <resea/task.h>
#include <resea/malloc.h>
#include <elf/elf.h>
#include <string.h>
#include "mm.h"
#include "task.h"
#include "pages.h"
#include "bootfs.h"

static vaddr_t tmp_page = 0;
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

void free_page_area(struct page_area *area) {
    pages_decref(paddr2pfn(area->paddr), area->num_pages);
    list_remove(&area->next);
    free(area);
}

static void free_pages(struct task *task, paddr_t paddr) {
    LIST_FOR_EACH(area, &task->page_areas, struct page_area, next) {
        if (area->paddr == paddr) {
            free_page_area(area);
            return;
        }
    }

    WARN("failed to free paddr=%p in %s (double free?)", paddr, task->name);
}

static bool is_mappable_paddr_range(paddr_t paddr, size_t num_pages) {
    paddr_t paddr_end = paddr + num_pages * PAGE_SIZE;
    return
        paddr >= PAGES_BASE_ADDR
        && paddr_end >= PAGES_BASE_ADDR
        && paddr_end < PAGES_BASE_ADDR_END
        && paddr < paddr_end // No wrapping around.
        ;
}

error_t alloc_phy_pages(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                        size_t num_pages) {
    *vaddr = alloc_virt_pages(task, num_pages);
    if (*paddr) {
        if (!IS_ALIGNED(*paddr, PAGE_SIZE)) {
            WARN_DBG("%s: unaligned paddr %p", __func__, *paddr);
            return ERR_INVALID_ARG;
        }

        if (!is_mappable_paddr_range(*paddr, num_pages)) {
            WARN_DBG("%s: invalid paddr %p", __func__, *paddr);
            return ERR_NOT_ACCEPTABLE;
        }

        // Map the specified physical memory address.
        for (size_t i = 0; i< num_pages; i++) {
            offset_t off = i * PAGE_SIZE;
            error_t err = map_page(task, *vaddr + off, *paddr + off, MAP_TYPE_READWRITE, false);
            if (err != OK) {
                return err;
            }
        }

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
    return pager(task, vaddr, 0, EXP_PF_USER | EXP_PF_WRITE /* FIXME: strip PF_WRITE */);
}

error_t map_page(struct task *task, vaddr_t vaddr, paddr_t paddr, unsigned flags,
                 bool overwrite) {
    if (overwrite) {
        vm_unmap(task->tid, vaddr);
    }

    // Calls vm_map multiple times because the kernel needs multiple memory
    // pages (kpage) for multi-level page table structures.
    while (true) {
        paddr_t kpage = alloc_pages(task, 0, 1);
        error_t err = vm_map(task->tid, vaddr, paddr, kpage, flags);
        switch (err) {
            case ERR_TRY_AGAIN:
                continue;
            default:
                if (err != OK) {
                    WARN_DBG("%s: failed to map a page: %s (paddr=%p, vaddr=%p, kpage=%p)",
                        task->name, err2str(err), paddr, vaddr, kpage
                    );
                }
                free_pages(task, kpage);
                return err;
        }
    }
}

paddr_t pager(struct task *task, vaddr_t vaddr, vaddr_t ip, unsigned fault) {
    if (vaddr < PAGE_SIZE) {
        WARN("%s (%d): null pointer dereference at vaddr=%p, ip=%p", task->name, task->tid, vaddr, ip);
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
        paddr_t paddr = alloc_pages(task, vaddr, 1);
        ASSERT_OK(map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
        memset((void *) tmp_page, 0, PAGE_SIZE);
        strncpy((void *) tmp_page, task->cmdline, PAGE_SIZE);
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
        ASSERT_OK(map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
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
            paddr_t paddr = alloc_pages(task, vaddr, 1);
            ASSERT_OK(map_page(vm_task, tmp_page, paddr, MAP_TYPE_READWRITE, false));
            size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
            read_file(task->file, offset_in_segment, (void *) tmp_page, PAGE_SIZE);
            return paddr;
        }
    }

    WARN("invalid memory access (addr=%p), killing %s...", vaddr_original, task->name);
    return 0;
}

void mm_init(void) {
    tmp_page = alloc_virt_pages(vm_task, 1);
}
