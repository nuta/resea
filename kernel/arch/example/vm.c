#include <arch.h>
#include <syscall.h>
#include <task.h>

error_t arch_vm_map(struct task *task, vaddr_t vaddr, paddr_t paddr,
                    paddr_t kpage, unsigned flags) {
    return OK;
}

error_t arch_vm_unmap(struct task *task, vaddr_t vaddr) {
    return OK;
}

paddr_t vm_resolve(struct task *task, vaddr_t vaddr) {
    return 0;
}

void arch_memcpy_from_user(void *dst, __user const void *src, size_t len) {
}

void arch_memcpy_to_user(__user void *dst, const void *src, size_t len) {
}
