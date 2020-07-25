#include <syscall.h>
#include <string.h>

error_t vm_create(struct vm *vm) {
    // Do nothing: we don't support virtual memory.
    return OK;
}

void vm_destroy(struct vm *vm) {
    // Do nothing: we don't support virtual memory.
}

error_t vm_link(struct vm *vm, vaddr_t vaddr, paddr_t paddr,
                pageattrs_t attrs) {
    // Do nothing: we don't support virtual memory.
    return OK;
}

paddr_t vm_resolve(struct vm *vm, vaddr_t vaddr) {
    return vaddr;
}

void arch_memcpy_from_user(void *dst, userptr_t src, size_t len) {
    memcpy(dst, (void *) src, len);
}
void arch_memcpy_to_user(userptr_t dst, const void *src, size_t len) {
    memcpy((void *) dst, src, len);
}
void arch_strncpy_from_user(char *dst, userptr_t src, size_t max_len) {
    strncpy(dst, (char *) src, max_len);
}
