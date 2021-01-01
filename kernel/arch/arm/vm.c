#include <arch.h>
#include <string.h>
#include <syscall.h>
#include <task.h>

error_t arch_vm_map(struct task *task, vaddr_t vaddr, paddr_t paddr,
                    paddr_t kpage, unsigned flags) {
    // Do nothing: we don't support virtual memory.
    return OK;
}

error_t arch_vm_unmap(struct task *task, vaddr_t vaddr) {
    // Do nothing: we don't support virtual memory.
    return OK;
}

paddr_t vm_resolve(struct task *task, vaddr_t vaddr) {
    return vaddr;
}

void arch_memcpy_from_user(void *dst, __user const void *src, size_t len) {
    memcpy(dst, (void *) src, len);
}

void arch_memcpy_to_user(__user void *dst, const void *src, size_t len) {
    memcpy((void *) dst, src, len);
}
