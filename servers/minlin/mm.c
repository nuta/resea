#include <resea/malloc.h>
#include <resea/task.h>
#include <string.h>
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "proc.h"

struct mchunk *mm_resolve(struct mm *mm, vaddr_t vaddr) {
    LIST_FOR_EACH(mchunk, &mm->mchunks, struct mchunk, next) {
        if (mchunk->vaddr <= vaddr && vaddr < mchunk->vaddr + mchunk->len) {
            return mchunk;
        }
    }

    return NULL;
}

struct mchunk *mm_alloc_mchunk(struct mm *mm, vaddr_t vaddr, size_t len) {
    DEBUG_ASSERT(IS_ALIGNED(len, PAGE_SIZE));

    struct message m;
    m.type = ALLOC_PAGES_MSG;
    m.alloc_pages.paddr = 0;
    m.alloc_pages.num_pages = len / PAGE_SIZE;
    error_t err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    ASSERT(m.type == ALLOC_PAGES_REPLY_MSG);

    struct mchunk *mchunk = malloc(sizeof(*mchunk));
    mchunk->vaddr = vaddr;
    mchunk->paddr = m.alloc_pages_reply.paddr;
    mchunk->buf = (void *) m.alloc_pages_reply.vaddr;
    mchunk->len = len;
    list_push_back(&mm->mchunks, &mchunk->next);
    return mchunk;
}

struct mchunk *mm_clone_mchunk(struct proc *child, struct mchunk *src) {
    struct mchunk *dst = mm_alloc_mchunk(&child->mm, src->vaddr, src->len);
    memcpy(dst->buf, src->buf, dst->len);
    return dst;
}

errno_t mm_fork(struct proc *parent, struct proc *child) {
    list_init(&child->mm.mchunks);
    if (parent) {
        LIST_FOR_EACH(mchunk, &parent->mm.mchunks, struct mchunk, next) {
            mm_clone_mchunk(child, mchunk);
        }
    }

    return 0;
}

error_t copy_from_user(struct proc *proc, void *dst, vaddr_t src,
                       size_t len) {
    size_t remaining = len;
    while (remaining > 0) {
        size_t copy_len = MIN(remaining, PAGE_SIZE - (src % PAGE_SIZE));
        vaddr_t src_aligned = ALIGN_DOWN(src, PAGE_SIZE);
        struct mchunk *mchunk = mm_resolve(&proc->mm, src_aligned);
        if (!mchunk) {
            handle_page_fault(proc, src, EXP_PF_USER);
            mchunk = mm_resolve(&proc->mm, src);
            DEBUG_ASSERT(mchunk);
        }

        memcpy(dst, &mchunk->buf[src - mchunk->vaddr], copy_len);
        dst += copy_len;
        src += copy_len;
        remaining -= copy_len;
    }

    return OK;
}

error_t copy_to_user(struct proc *proc, vaddr_t dst, const void *src,
                     size_t len) {
    size_t remaining = len;
    while (remaining > 0) {
        size_t copy_len = MIN(remaining, PAGE_SIZE - (dst % PAGE_SIZE));
        vaddr_t dst_aligned = ALIGN_DOWN(dst, PAGE_SIZE);
        struct mchunk *mchunk = mm_resolve(&proc->mm, dst_aligned);
        if (!mchunk) {
            if (!handle_page_fault(proc, dst, EXP_PF_USER | EXP_PF_WRITE)) {
                return ERR_NOT_PERMITTED;
            }

            mchunk = mm_resolve(&proc->mm, dst);
            DEBUG_ASSERT(mchunk);
        }

        memcpy(&mchunk->buf[dst - mchunk->vaddr], src, copy_len);
        dst += copy_len;
        src += copy_len;
        remaining -= copy_len;
    }

    return OK;
}

size_t strncpy_from_user(struct proc *proc, char *dst, vaddr_t src,
                         size_t max_len) {
    size_t read_len = 0;
    while (read_len < max_len) {
        char ch;
        error_t err;
        if ((err = copy_from_user(proc, &ch, src, sizeof(ch))) != OK) {
            return 0;
        }

        dst[read_len] = ch;
        if (!ch) {
            return read_len;
        }

        read_len++;
        src++;
    }

    return read_len;
}

volatile unsigned long long x = 123; // TODO: remove me

static error_t map_page(struct proc *proc, vaddr_t vaddr, vaddr_t paddr,
                        unsigned flags, bool overwrite) {
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    flags |= overwrite ? (MAP_DELETE | MAP_UPDATE) : MAP_UPDATE;
    while (true) {
        struct mchunk *m = mm_alloc_mchunk(&proc->mm, vaddr, PAGE_SIZE);
        x += *((uint8_t *) paddr); // Handle page faults in advance. FIXME: remove me
        x += *((uint8_t *) m->buf); // Handle page faults in advance. FIXME: remove me
        error_t err = task_map(proc->task, vaddr, paddr, (vaddr_t) m->buf, flags);
        switch (err) {
            case ERR_TRY_AGAIN:
                continue;
            default:
                return err;
        }
    }
}

static vaddr_t fill_page(struct proc *proc, vaddr_t vaddr, unsigned fault) {
    vaddr_t aligned_vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

    if (fault & EXP_PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (perhaps segfault?)", proc->name,
             vaddr);
        return 0;
    }

    // Look for the associated mchunk.
    struct mchunk *mchunk = mm_resolve(&proc->mm, vaddr);
    if (mchunk) {
        return (vaddr_t) mchunk->buf + (aligned_vaddr - mchunk->vaddr);
    }

    // Allocate heap.
    if (HEAP_ADDR <= vaddr && vaddr < proc->current_brk) {
        struct mchunk *new_mchunk = mm_alloc_mchunk(&proc->mm, aligned_vaddr, PAGE_SIZE);
        if (!new_mchunk) {
            return 0;
        }

        memset(new_mchunk->buf, 0, new_mchunk->len);
        return (vaddr_t) new_mchunk->buf + (aligned_vaddr - new_mchunk->vaddr);
    }

    // Look for the associated program header.
    struct elf64_phdr *phdr = NULL;
    for (unsigned i = 0; i < proc->ehdr->e_phnum; i++) {
        // TODO: .bss section

        // Ignore GNU_STACK
        if (!proc->phdrs[i].p_vaddr) {
            continue;
        }

        vaddr_t start = proc->phdrs[i].p_vaddr;
        vaddr_t end = start + proc->phdrs[i].p_memsz;
        if (start <= vaddr && vaddr < end) {
            phdr = &proc->phdrs[i];
            break;
        }
    }

    if (!phdr) {
        WARN("invalid memory access (addr=%p), killing %s...", vaddr, proc->name);
        return 0;
    }

    // Allocate a page and fill it with the file data.
    struct mchunk *new_mchunk = mm_alloc_mchunk(&proc->mm, aligned_vaddr, PAGE_SIZE);
    if (!new_mchunk) {
        return 0;
    }

    memset(new_mchunk->buf, 0, PAGE_SIZE);
    // Copy file contents.
    size_t offset_in_file;
    size_t offset_in_page;
    size_t copy_len;
    if (aligned_vaddr < phdr->p_vaddr) {
        offset_in_file = phdr->p_offset;
        offset_in_page = phdr->p_vaddr % PAGE_SIZE;
        copy_len = MIN(PAGE_SIZE - offset_in_page, phdr->p_filesz);
    } else {
        size_t offset_in_segment = aligned_vaddr - phdr->p_vaddr;
        if (offset_in_segment >= phdr->p_filesz) {
            // The entire page should be and have already been filled with
            // zeroes (.bss section).
            copy_len = 0;
            offset_in_file = 0;
            offset_in_page = 0;
        } else {
            offset_in_file = offset_in_segment + phdr->p_offset;
            offset_in_page = 0;
            copy_len = MIN(phdr->p_filesz - offset_in_segment, PAGE_SIZE);
        }
    }

    fs_read_pos(proc->exec, &new_mchunk->buf[offset_in_page],
                offset_in_file, copy_len);

    return (vaddr_t) new_mchunk->buf;
}

error_t handle_page_fault(struct proc *proc, vaddr_t vaddr, unsigned fault) {
    vaddr_t target = fill_page(proc, vaddr, fault);
    if (!target) {
        WARN_DBG("failed to fill a page for %s", proc->name);
        // TODO: Kill the proc.
        return ERR_ABORTED;
    }

    vaddr_t aligned_vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);
    ASSERT_OK(map_page(proc, aligned_vaddr, target, MAP_W /* TODO: */, false));
    return OK;
}
