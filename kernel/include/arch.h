#ifndef __ARCH_H__
#define __ARCH_H__

#include <arch_types.h>
#include <types.h>

struct bootinfo;
void arch_init(struct bootinfo *args);
void arch_idle(void);
void arch_putchar(char ch);
NORETURN void arch_panic(void);
void arch_mp_init(void);

void page_table_init(struct page_table *pt);
void page_table_destroy(struct page_table *pt);
void link_page(struct page_table *pt, vaddr_t vaddr, paddr_t paddr,
                    int num_pages, uintmax_t flags);
void unlink_page(struct page_table *pt, vaddr_t vaddr, int num_pages);
paddr_t resolve_paddr_from_vaddr(struct page_table *pt,
                                 vaddr_t vaddr);
void enable_irq(uint8_t irq);
uint64_t arch_read_ioport(uintmax_t addr, int size);
void arch_write_ioport(uintmax_t addr, int size, uint64_t data);

struct thread;
error_t arch_thread_init(struct thread *thread, vaddr_t start, vaddr_t stack,
                         vaddr_t kernel_stack, vaddr_t user_buffer,
                         bool is_kernel_thread);
void arch_thread_destroy(struct thread *thread);
inline struct thread *get_current_thread(void);
void thread_allow_io(struct thread *thread);
void set_current_thread(struct thread *thread);
void arch_thread_switch(struct thread *prev, struct thread *next);

inline bool is_valid_page_base_addr(vaddr_t page_base);

char *strcpy(char *dst, size_t dst_len, const char *src);
int strcmp(const char *s1, const char *s2);

// ASan requries memset/memcpy to be compatible with the C standard library. We
// prepend "inlined_" to avoid the issue.
inline void *inlined_memset(void *dst, int ch, size_t len);
inline void *inlined_memcpy(void *dst, void *src, size_t len);

inline vaddr_t arch_get_stack_pointer(void);
bool arch_asan_is_kernel_address(vaddr_t addr);
void arch_asan_init(void);

int arch_debugger_readchar(void);

#endif
