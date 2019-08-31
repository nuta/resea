#ifndef __ARCH_H__
#define __ARCH_H__

#include <arch_types.h>
#include <types.h>

#define CPUVAR (x64_get_cpuvar())

void arch_init(void);
void arch_idle(void);
NORETURN void arch_panic(void);
void arch_putchar(char ch);
void page_table_init(struct page_table *pt);
void page_table_destroy(struct page_table *pt);

struct thread;
void arch_thread_init(struct thread *thread, vaddr_t start, vaddr_t stack,
                      vaddr_t kernel_stack, vaddr_t user_buffer,
                      bool is_kernel_thread);
struct thread *arch_get_current_thread(void);
void arch_set_current_thread(struct thread *thread);
void arch_thread_switch(struct thread *prev, struct thread *next);
void arch_link_page(struct page_table *pt, vaddr_t vaddr, paddr_t paddr,
                    int num_pages, uintmax_t flags);
paddr_t arch_resolve_paddr_from_vaddr(struct page_table *pt,
                                      vaddr_t vaddr);
void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
static inline bool spin_try_lock(spinlock_t *lock);
static inline void spin_unlock(spinlock_t *lock);
flags_t spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, flags_t flags);
vaddr_t arch_get_stack_pointer(void);

void *memset(void *dst, size_t dst_len, char ch, size_t len);
void *memcpy(void *dst, size_t dst_len, void *src, size_t copy_len);
static inline void *memcpy_unchecked(void *dst, void *src, size_t len);
char *strcpy(char *dst, size_t dst_len, const char *src);

#endif
