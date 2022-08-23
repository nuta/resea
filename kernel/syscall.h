#define __failable
#define __user
typedef int task_t;
typedef int error_t;
typedef int uaddr_t;
typedef int paddr_t;
typedef int notifications_t;

struct message {
};

__failable task_t task_create(uaddr_t ip, uaddr_t name, task_t pager, unsigned flags);
error_t task_destroy(task_t task);
error_t task_exit(void);
__failable task_t task_self(void);

error_t ipc(task_t dst, task_t src, __user struct message *m, unsigned flags);
error_t notify(task_t dst, notifications_t notifications);

error_t timer_set(unsigned ms);
error_t timer_clear(unsigned ms);

__failable paddr_t pm_alloc(int num_pages);
error_t pm_free(paddr_t paddr, int num_pages);

error_t vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, int num_pages);
error_t vm_unmap(task_t task, uaddr_t uaddr, int num_pages);

error_t irq_acquire(int irq);
error_t irq_release(int irq);

__failable int sys_console_read(uaddr_t buf, int buf_len);
__failable int sys_console_write(uaddr_t buf, int len);
