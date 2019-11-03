.intel_syntax noprefix

/* void *memcpy(void *dst, const void *src, size_t n); */
.globl memcpy
memcpy:
    mov rcx, rdx
    mov rax, rdi
    cld
    rep movsb
    ret

/* void *memmove(void *dst, const void *src, size_t n); */
.globl memmove
memmove:
    mov rcx, rdx
    mov rax, rdi
    cmp rdi, rsi
    ja 1f
    cld /* Copy forwards. */
    rep movsb
    ret
1:
    add rdi, rcx
    add rsi, rcx
    sub rdi, 1
    sub rsi, 1
    std /* Copy backwards. */
    rep movsb
    ret

/* void *memset(void *ptr, int value, size_t n); */
.globl memset
memset:
    mov rcx, rdx
    mov r8, rdi
    mov al, sil
    cld
    rep stosb
    mov rax, r8
    ret

/* void *memcmp(void *ptr1, void *ptr2, size_t n); */
.globl memcmp
memcmp:
    mov rcx, rdx
    cld
    repe cmpsb
    jne 1f
    xor rax, rax
    ret

1:
    mov al, [rdi - 1]
    sub al, [rsi - 1]
    ret
