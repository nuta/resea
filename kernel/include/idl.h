#ifndef __IDL_H__
#define __IDL_H__

#define STRING_LEN_MAX 128
typedef char string_t[STRING_LEN_MAX];

#define MSG_REPLY_FLAG (1ULL << 7)
#define RUNTIME_INTERFACE  1ULL
#define RUNTIME_EXIT_INLINE_LEN (sizeof(int32_t))
#define RUNTIME_EXIT_MSG     (         (((RUNTIME_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (RUNTIME_EXIT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_exit_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    int32_t code;
} PACKED;
#define RUNTIME_EXIT_REPLY_INLINE_LEN (0)
#define RUNTIME_EXIT_REPLY_MSG     (         (((RUNTIME_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (RUNTIME_EXIT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_exit_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define RUNTIME_PRINTCHAR_INLINE_LEN (sizeof(uint8_t))
#define RUNTIME_PRINTCHAR_MSG     (         (((RUNTIME_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (RUNTIME_PRINTCHAR_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_printchar_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    uint8_t ch;
} PACKED;
#define RUNTIME_PRINTCHAR_REPLY_INLINE_LEN (0)
#define RUNTIME_PRINTCHAR_REPLY_MSG     (         (((RUNTIME_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (RUNTIME_PRINTCHAR_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_printchar_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;

#define RUNTIME_PRINT_STR_INLINE_LEN (STRING_LEN_MAX)
#define RUNTIME_PRINT_STR_MSG     (         (((RUNTIME_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (RUNTIME_PRINT_STR_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_print_str_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    string_t str;
} PACKED;

#define PROCESS_INTERFACE  2ULL
#define PROCESS_CREATE_INLINE_LEN (STRING_LEN_MAX)
#define PROCESS_CREATE_MSG     (         (((PROCESS_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (PROCESS_CREATE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_create_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    string_t name;
} PACKED;
#define PROCESS_CREATE_REPLY_INLINE_LEN (sizeof(pid_t))
#define PROCESS_CREATE_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET) | MSG_CHANNEL_PAYLOAD | (PROCESS_CREATE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_create_reply_payload {
    cid_t pager_ch;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
} PACKED;
#define PROCESS_DESTROY_INLINE_LEN (sizeof(pid_t))
#define PROCESS_DESTROY_MSG     (         (((PROCESS_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (PROCESS_DESTROY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_destroy_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
} PACKED;
#define PROCESS_DESTROY_REPLY_INLINE_LEN (0)
#define PROCESS_DESTROY_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (PROCESS_DESTROY_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_destroy_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define PROCESS_ADD_PAGER_INLINE_LEN (sizeof(pid_t) + sizeof(uintptr_t) + sizeof(size_t) + sizeof(uint8_t))
#define PROCESS_ADD_PAGER_MSG     (         (((PROCESS_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (PROCESS_ADD_PAGER_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_add_vm_area_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
    uintptr_t start;
    size_t size;
    uint8_t flags;
} PACKED;
#define PROCESS_ADD_PAGER_REPLY_INLINE_LEN (0)
#define PROCESS_ADD_PAGER_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| (PROCESS_ADD_PAGER_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_add_vm_area_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define PROCESS_SEND_CHANNEL_INLINE_LEN (sizeof(pid_t))
#define PROCESS_SEND_CHANNEL_MSG     (         (((PROCESS_INTERFACE << 8) | 4ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (PROCESS_SEND_CHANNEL_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_send_channel_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
} PACKED;
#define PROCESS_SEND_CHANNEL_REPLY_INLINE_LEN (0)
#define PROCESS_SEND_CHANNEL_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 4) << MSG_TYPE_OFFSET)| (PROCESS_SEND_CHANNEL_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_send_channel_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define THREAD_INTERFACE  3ULL
#define THREAD_SPAWN_INLINE_LEN (sizeof(pid_t) + sizeof(uintptr_t) + sizeof(uintptr_t) + sizeof(uintptr_t) + sizeof(uintptr_t))
#define THREAD_SPAWN_MSG     (         (((THREAD_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (THREAD_SPAWN_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_spawn_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
    uintptr_t start;
    uintptr_t stack;
    uintptr_t buffer;
    uintptr_t arg;
} PACKED;
#define THREAD_SPAWN_REPLY_INLINE_LEN (sizeof(tid_t))
#define THREAD_SPAWN_REPLY_MSG     (         (((THREAD_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (THREAD_SPAWN_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_spawn_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    tid_t tid;
} PACKED;
#define THREAD_DESTROY_INLINE_LEN (sizeof(tid_t))
#define THREAD_DESTROY_MSG     (         (((THREAD_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (THREAD_DESTROY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_destroy_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    tid_t tid;
} PACKED;
#define THREAD_DESTROY_REPLY_INLINE_LEN (0)
#define THREAD_DESTROY_REPLY_MSG     (         (((THREAD_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (THREAD_DESTROY_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_destroy_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define TIMER_INTERFACE  4ULL
typedef int32_t timer_id_t;
#define TIMER_SET_INLINE_LEN (sizeof(int32_t) + sizeof(int32_t))
#define TIMER_SET_MSG     (         (((TIMER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (TIMER_SET_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_set_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t __num_pages;
    int32_t initial;
    int32_t interval;
} PACKED;
#define TIMER_SET_REPLY_INLINE_LEN (sizeof(int32_t))
#define TIMER_SET_REPLY_MSG     (         (((TIMER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (TIMER_SET_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_set_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    int32_t timer;
} PACKED;
#define TIMER_CLEAR_INLINE_LEN (sizeof(int32_t))
#define TIMER_CLEAR_MSG     (         (((TIMER_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (TIMER_CLEAR_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_clear_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    int32_t timer;
} PACKED;
#define TIMER_CLEAR_REPLY_INLINE_LEN (0)
#define TIMER_CLEAR_REPLY_MSG     (         (((TIMER_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (TIMER_CLEAR_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_clear_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define KERNEL_INTERFACE  9ULL
#define SERVER_INTERFACE  10ULL
#define SERVER_CONNECT_INLINE_LEN (sizeof(uint8_t))
#define SERVER_CONNECT_MSG     (         (((SERVER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (SERVER_CONNECT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct server_connect_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    uint8_t interface;
} PACKED;
#define SERVER_CONNECT_REPLY_INLINE_LEN (0)
#define SERVER_CONNECT_REPLY_MSG     (         (((SERVER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (SERVER_CONNECT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct server_connect_reply_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define MEMORY_INTERFACE  11ULL
#define MEMORY_ALLOC_PAGES_INLINE_LEN (sizeof(size_t))
#define MEMORY_ALLOC_PAGES_MSG     (         (((MEMORY_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (MEMORY_ALLOC_PAGES_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_pages_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    size_t order;
} PACKED;
#define MEMORY_ALLOC_PAGES_REPLY_INLINE_LEN (0)
#define MEMORY_ALLOC_PAGES_REPLY_MSG     (         (((MEMORY_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (MEMORY_ALLOC_PAGES_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_pages_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t __num_pages;
} PACKED;
#define MEMORY_ALLOC_PHY_PAGES_INLINE_LEN (sizeof(paddr_t) + sizeof(size_t))
#define MEMORY_ALLOC_PHY_PAGES_MSG     (         (((MEMORY_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (MEMORY_ALLOC_PHY_PAGES_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_phy_pages_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    paddr_t map_at;
    size_t order;
} PACKED;
#define MEMORY_ALLOC_PHY_PAGES_REPLY_INLINE_LEN (sizeof(paddr_t))
#define MEMORY_ALLOC_PHY_PAGES_REPLY_MSG     (         (((MEMORY_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (MEMORY_ALLOC_PHY_PAGES_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_phy_pages_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t __num_pages;
    paddr_t paddr;
} PACKED;
#define PAGER_INTERFACE  12ULL
#define PAGER_FILL_INLINE_LEN (sizeof(pid_t) + sizeof(uintptr_t) + sizeof(size_t))
#define PAGER_FILL_MSG     (         (((PAGER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (PAGER_FILL_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pager_fill_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
    pid_t pid;
    uintptr_t addr;
    size_t num_pages;
} PACKED;
#define PAGER_FILL_REPLY_INLINE_LEN (0)
#define PAGER_FILL_REPLY_MSG     (         (((PAGER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (PAGER_FILL_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pager_fill_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t __num_pages;
} PACKED;
#define IO_INTERFACE  15ULL
#define IO_ALLOW_IOMAPPED_IO_INLINE_LEN (0)
#define IO_ALLOW_IOMAPPED_IO_MSG     (         (((IO_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (IO_ALLOW_IOMAPPED_IO_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_allow_iomapped_io_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define IO_ALLOW_IOMAPPED_IO_REPLY_INLINE_LEN (0)
#define IO_ALLOW_IOMAPPED_IO_REPLY_MSG     (         (((IO_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (IO_ALLOW_IOMAPPED_IO_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_allow_iomapped_io_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define IO_LISTEN_IRQ_INLINE_LEN (sizeof(uint8_t))
#define IO_LISTEN_IRQ_MSG     (         (((IO_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (IO_LISTEN_IRQ_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_listen_irq_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t __num_pages;
    uint8_t irq;
} PACKED;
#define IO_LISTEN_IRQ_REPLY_INLINE_LEN (0)
#define IO_LISTEN_IRQ_REPLY_MSG     (         (((IO_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (IO_LISTEN_IRQ_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_listen_irq_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define BENCHMARK_INTERFACE  19ULL
#define BENCHMARK_NOP_INLINE_LEN (0)
#define BENCHMARK_NOP_MSG     (         (((BENCHMARK_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (BENCHMARK_NOP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct benchmark_nop_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;
#define BENCHMARK_NOP_REPLY_INLINE_LEN (0)
#define BENCHMARK_NOP_REPLY_MSG     (         (((BENCHMARK_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (BENCHMARK_NOP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct benchmark_nop_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t __num_pages;
} PACKED;

#define IDL_MESSAGE_PAYLOADS \
    union { \
    struct runtime_exit_payload exit; \
    struct runtime_exit_reply_payload exit_reply; \
    struct runtime_printchar_payload printchar; \
    struct runtime_printchar_reply_payload printchar_reply; \
    struct runtime_print_str_payload print_str; \
    } runtime; \
    union { \
    struct process_create_payload create; \
    struct process_create_reply_payload create_reply; \
    struct process_destroy_payload destroy; \
    struct process_destroy_reply_payload destroy_reply; \
    struct process_add_vm_area_payload add_vm_area; \
    struct process_add_vm_area_reply_payload add_vm_area_reply; \
    struct process_send_channel_payload send_channel; \
    struct process_send_channel_reply_payload send_channel_reply; \
    } process; \
    union { \
    struct thread_spawn_payload spawn; \
    struct thread_spawn_reply_payload spawn_reply; \
    struct thread_destroy_payload destroy; \
    struct thread_destroy_reply_payload destroy_reply; \
    } thread; \
    union { \
    struct timer_set_payload set; \
    struct timer_set_reply_payload set_reply; \
    struct timer_clear_payload clear; \
    struct timer_clear_reply_payload clear_reply; \
    } timer; \
    union { \
    } kernel; \
    union { \
    struct server_connect_payload connect; \
    struct server_connect_reply_payload connect_reply; \
    } server; \
    union { \
    struct pager_fill_payload fill; \
    struct pager_fill_reply_payload fill_reply; \
    } pager; \
    union { \
    struct io_allow_iomapped_io_payload allow_iomapped_io; \
    struct io_allow_iomapped_io_reply_payload allow_iomapped_io_reply; \
    struct io_listen_irq_payload listen_irq; \
    struct io_listen_irq_reply_payload listen_irq_reply; \
    } io; \

#endif
