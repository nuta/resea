#define __user
typedef unsigned char uint8_t;
typedef int task_t;
typedef int error_t;
typedef int uaddr_t;
typedef int paddr_t;
typedef int notifications_t;
typedef int caps_t;

struct message {
    unsigned tag;
    task_t src;
    union {
        uint8_t data[56];

    };
};

task_t task_create(uaddr_t ip, uaddr_t name, task_t pager, unsigned flags);
error_t task_destroy(task_t task);
error_t task_exit(void);
task_t task_self(void);

caps_t caps_get(task_t task);
error_t caps_set(task_t task, caps_t caps);

task_t ipc(task_t dst, task_t src, __user struct message *m, unsigned flags);
error_t notify(task_t dst, notifications_t notifications);

error_t timer_set(unsigned ms);
error_t timer_clear(unsigned ms);

paddr_t pm_alloc(int num_pages);
error_t pm_free(paddr_t paddr, int num_pages);

error_t vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, int num_pages);
error_t vm_unmap(task_t task, uaddr_t uaddr, int num_pages);

error_t irq_acquire(int irq);
error_t irq_release(int irq);

int console_read(uaddr_t buf, int buf_len);
int console_write(uaddr_t buf, int len);

int kdebug(uaddr_t cmd, uaddr_t out, int out_len);
