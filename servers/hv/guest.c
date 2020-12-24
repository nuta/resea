#include <list.h>
#include <resea/printf.h>
#include <resea/task.h>
#include <resea/malloc.h>
#include <resea/ipc.h>
#include <resea/async.h>
#include <string.h>
#include <elf/elf.h>
#include "guest.h"
#include "mm.h"
#include "pci.h"
#include "virtio_blk.h"

static list_t guests;

#define ELF_NOTE_PVH_ENTRY 18

error_t load_kernel_elf(struct guest *guest, uint8_t *elf, size_t elf_len,
                        enum image_type *image_type) {
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) elf;
    *image_type = IMAGE_TYPE_PLAIN;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        WARN_DBG("invalid ELF magic");
        return ERR_NOT_ACCEPTABLE;
    }

    // Map associated program headers.
    struct elf64_phdr *phdrs =
        (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    for (unsigned i = 0; i < ehdr->e_phnum; i++) {
        // Ignore GNU_STACK
        if (!phdrs[i].p_vaddr) {
            continue;
        }

        if (phdrs[i].p_type == PT_NOTE) {
            offset_t offset = 0;
            while (offset < phdrs[i].p_filesz) {
                struct elf64_nhdr *nhdr =
                    (struct elf64_nhdr *) ((uintptr_t) elf + phdrs[i].p_offset + offset);
                vaddr_t contents = (vaddr_t) nhdr + sizeof(*nhdr) + nhdr->n_namesz;
                if (nhdr->n_type == ELF_NOTE_PVH_ENTRY) {
                    uint32_t entry = *((uint32_t *) contents);
                    TRACE("found the Xen PVH header: entry=%p", entry);
                    guest->entry = entry;
                    *image_type = IMAGE_TYPE_XEN_PVH;
                }

                offset += sizeof(*nhdr) + ALIGN_UP(nhdr->n_namesz, 4)
                    + ALIGN_UP(nhdr->n_descsz, 4);
            }

            continue;
        }

        if (phdrs[i].p_filesz > 0) {
            struct mapping *m = malloc(sizeof(*m));
            m->gpaddr = phdrs[i].p_paddr;
            m->len = phdrs[i].p_filesz;
            m->ops = &file_mapping_ops;
            m->file.data = (uint8_t *) ((uintptr_t) elf + phdrs[i].p_offset);
            list_push_back(&guest->mappings, &m->next);
            TRACE("guest ELF: gpaddr=%p, len=%p", m->gpaddr, m->len);
        }
    }

    if (image_type == IMAGE_TYPE_PLAIN) {
        guest->entry = ehdr->e_entry;
    }

    return OK;
}

struct guest *guest_lookup(task_t task) {
    LIST_FOR_EACH(guest, &guests, struct guest, next) {
        if (guest->task == task) {
            return guest;
        }
    }

    PANIC("unknown task ID %d", task);
}

static void add_mapping(struct guest *guest, gpaddr_t base, size_t len,
                        struct mapping_ops *ops) {
    struct mapping *m = malloc(sizeof(*m));
    m->gpaddr = base;
    m->len = len;
    m->ops = ops;
    list_push_back(&guest->mappings, &m->next);
}

task_t alloc_task(void) {
    struct message m;
    m.type = TASK_ALLOC_MSG;
    m.task_alloc.pager = task_self();
    error_t err = ipc_call(INIT_TASK, &m);
    if (err != OK) {
        return err;
    }

    return m.task_alloc_reply.task;
}

error_t guest_create(uint8_t *elf, size_t elf_len) {
    task_t tid = alloc_task();
    if (IS_ERROR(tid)) {
        return tid;
    }

    struct guest *guest = malloc(sizeof(*guest));
    list_init(&guest->mappings);
    list_init(&guest->page_areas);
    guest->task = tid;
    guest->halted = false;
    guest->pending_irq_bitmap = 0;

    enum image_type image_type;
    error_t err = load_kernel_elf(guest, (void *) elf, elf_len, &image_type);
    if (err != OK) {
        WARN_DBG("failed to load the kernel image (%s)", err2str(err));
        return err;
    }

    paddr_t ept_pml4_paddr;
    uint64_t *ept_pml4 = alloc_internal_page(guest, 1, &ept_pml4_paddr);
    bzero(ept_pml4, PAGE_SIZE);
    guest->ept_pml4 = ept_pml4;
    guest->ept_pml4_paddr = ept_pml4_paddr;
    bzero(&guest->initial_regs, sizeof(guest->initial_regs));

    add_mapping(guest, 0xf0000, 0x1000, &misc_mapping_ops);
    add_mapping(guest, 0xfee00000, PAGE_SIZE, &lapic_mapping_ops);
    switch (image_type) {
        case IMAGE_TYPE_PLAIN:
            break;
        case IMAGE_TYPE_XEN_PVH:
            add_mapping(guest, 0x5000, PAGE_SIZE, &pvh_mapping_ops);
            guest->initial_regs.rbx = 0x5000;
            break;
    }

    pci_init(guest);
    virtio_blk_init(guest);

    err = task_create(guest->task, "hv_guest", 0, task_self(), TASK_HV);
    if (err != OK) {
        WARN_DBG("failed to create hypervisor guest task (%s)", err2str(err));
        return err;
    }

    list_push_back(&guests, &guest->next);
    return OK;
}

void guest_inject_irq(struct guest *guest, int irq) {
    DEBUG_ASSERT(irq < 16);
    guest->pending_irq_bitmap |= 1 << irq;
}

void guest_tick(void) {
    LIST_FOR_EACH(guest, &guests, struct guest, next) {
        guest->pending_irq_bitmap |= 1 /* timer IRQ */;
        if (guest->halted) {
            struct message m;
            bzero(&m, sizeof(m));
            m.type = HV_HALT_REPLY_MSG;
            ipc_reply(guest->task, &m);
            guest->halted = false;
        }
    }
}

void guest_send_pending_events(void) {
    LIST_FOR_EACH(guest, &guests, struct guest, next) {
        if (guest->pending_irq_bitmap) {
            ipc_notify(guest->task, NOTIFY_ASYNC);
            if (guest->halted) {
                struct message m;
                bzero(&m, sizeof(m));
                m.type = HV_HALT_REPLY_MSG;
                ipc_reply(guest->task, &m);
                guest->halted = false;
            }
        }
    }
}

void guest_init(void) {
    list_init(&guests);
}
