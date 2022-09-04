#include "page_fault.h"
#include "task.h"
#include <print_macros.h>>

/// Tries to fill a page at `vaddr` for the task. Returns the allocated physical
/// memory address on success or 0 on failure.
error_t handle_page_fault(struct task *task, uaddr_t vaddr, uaddr_t ip,
                          unsigned fault) {
    if (vaddr < PAGE_SIZE) {
        WARN("%s (%d): null pointer dereference at vaddr=%p, ip=%p", task->name,
             task->tid, vaddr, ip);
        return ERR_ABORTED;  // FIXME:
    }

    uaddr_t vaddr_original = vaddr;
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

    if (fault & EXP_PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (IP=%p, perhaps segfault?)",
             task->name, vaddr_original, ip);
        return ERR_ABORTED;  // FIXME:
    }

    // Look for the associated program header.
    struct elf64_phdr *phdr = NULL;
    if (task->ehdr) {
        for (unsigned i = 0; i < task->ehdr->e_phnum; i++) {
            // Ignore GNU_STACK
            if (!task->phdrs[i].p_vaddr) {
                continue;
            }

            uaddr_t start = task->phdrs[i].p_vaddr;
            uaddr_t end = start + task->phdrs[i].p_memsz;
            if (start <= vaddr && vaddr <= end) {
                phdr = &task->phdrs[i];
                break;
            }
        }

        if (phdr) {
            // Allocate a page and fill it with the file data.
            paddr_t paddr = 0;
            if (task_page_alloc(task, &vaddr, &paddr, 1) != OK) {
                return ERR_ABORTED;  // FIXME:
            }

            // TODO:
            // vaddr_t aligned_vaddr = ALIGN_DOWN(m.page_fault.uaddr,
            // PAGE_SIZE);
            //
            // size_t offset_in_segment = (vaddr - phdr->p_vaddr) +
            // phdr->p_offset; read_file(task->file, offset_in_segment, (void *)
            // tmp_page,
            //           PAGE_SIZE);
            // return paddr;
        }
    }

    WARN("invalid memory access (addr=%p, IP=%p), killing %s...",
         vaddr_original, ip, task->name);
    return 0;
}
