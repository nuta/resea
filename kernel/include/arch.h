#ifndef __ARCH_H__
#define __ARCH_H__

#include <arch_types.h>
#include <types.h>

#define CPUVAR (x64_get_cpuvar())

struct init_args;
void arch_init(struct init_args *args);
void arch_idle(void);
void arch_putchar(char ch);
NORETURN void arch_panic(void);
NORETURN void arch_poweroff(void);

void page_table_init(struct page_table *pt);
void page_table_destroy(struct page_table *pt);
void link_page(struct page_table *pt, vaddr_t vaddr, paddr_t paddr,
                    int num_pages, uintmax_t flags);
void unlink_page(struct page_table *pt, vaddr_t vaddr, int num_pages);
paddr_t resolve_paddr_from_vaddr(struct page_table *pt,
                                 vaddr_t vaddr);
void enable_irq(uint8_t irq);

struct thread;
error_t arch_thread_init(struct thread *thread, vaddr_t start, vaddr_t stack,
                         vaddr_t kernel_stack, vaddr_t user_buffer,
                         bool is_kernel_thread);
static inline struct thread *get_current_thread(void);
void thread_allow_io(struct thread *thread);
void set_current_thread(struct thread *thread);
void arch_thread_switch(struct thread *prev, struct thread *next);

static inline bool is_valid_page_base_addr(vaddr_t page_base);

void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
static inline bool spin_try_lock(spinlock_t *lock);
static inline void spin_unlock(spinlock_t *lock);
flags_t spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, flags_t flags);

char *strcpy(char *dst, size_t dst_len, const char *src);

// ASan requries memset/memcpy to be compatible with the C standard library. We
// prepend "inlined_" to avoid the issue.
static inline void *inlined_memset(void *dst, int ch, size_t len);
static inline void *inlined_memcpy(void *dst, void *src, size_t len);

static inline vaddr_t arch_get_stack_pointer(void);
bool arch_asan_is_kernel_address(vaddr_t addr);
void arch_asan_init(void);

#endif
