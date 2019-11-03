#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <types.h>

//
//  Syscall numbers and flags.
//
#define SYSCALL_IPC 0
#define SYSCALL_OPEN 1
#define SYSCALL_CLOSE 2
#define SYSCALL_LINK 3
#define SYSCALL_TRANSFER 4
#define SYSCALL_NOTIFY 5
#define SYSCALL_NOP 6

#define IPC_SEND (1ULL << 8)
#define IPC_RECV (1ULL << 9)
#define IPC_NOBLOCK (1ULL << 10)
#define SYSCALL_TYPE(syscall_id) ((syscall_id) &0xff)

//
//  Message Header.
//
typedef uint32_t header_t;
#define MSG_INLINE_LEN_OFFSET 0
#define MSG_TYPE_OFFSET 16
#define MSG_PAGE_PAYLOAD (1ULL << 11)
#define MSG_CHANNEL_PAYLOAD (1ULL << 12)
#define INLINE_PAYLOAD_LEN(header) (((header) >> MSG_INLINE_LEN_OFFSET) & 0x7ff)
#define INTERFACE_ID(header) ((((header) >> MSG_TYPE_OFFSET) & 0xffff) >> 8)
#define MSG_COMMON_HEADER_LEN  32
#define MSG_MAX_LEN            256
#define INLINE_PAYLOAD_LEN_MAX (MSG_MAX_LEN - MSG_COMMON_HEADER_LEN)
#define ERROR_TO_HEADER(error) ((uint32_t) (-error) << MSG_TYPE_OFFSET)

//
//  Notification.
//
#define NOTIFICATION_INTERFACE  100ULL
#define NOTIFICATION_MSG        (((NOTIFICATION_INTERFACE << 8) | 1) << MSG_TYPE_OFFSET)

typedef uint32_t notification_t;
#define NOTIFY_INTERRUPT (1UL << 0)
#define NOTIFY_TIMER     (1UL << 1)

//
//  Message
//
#define SMALLSTRING_LEN_MAX 128
typedef char smallstring_t[SMALLSTRING_LEN_MAX];

#define MSG_REPLY_FLAG (1ULL << 7)
#define RUNTIME_INTERFACE  1ULL
#define RUNTIME_EXIT_INLINE_LEN (sizeof(int32_t))
#define RUNTIME_EXIT_MSG     (         (((RUNTIME_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (RUNTIME_EXIT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_exit_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t code;
} PACKED;
#define RUNTIME_EXIT_REPLY_INLINE_LEN (0)
#define RUNTIME_EXIT_REPLY_MSG     (         (((RUNTIME_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (RUNTIME_EXIT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_exit_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define RUNTIME_PRINTCHAR_INLINE_LEN (sizeof(uint8_t))
#define RUNTIME_PRINTCHAR_MSG     (         (((RUNTIME_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (RUNTIME_PRINTCHAR_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_printchar_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t ch;
} PACKED;
#define RUNTIME_PRINTCHAR_REPLY_INLINE_LEN (0)
#define RUNTIME_PRINTCHAR_REPLY_MSG     (         (((RUNTIME_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (RUNTIME_PRINTCHAR_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct runtime_printchar_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define PROCESS_INTERFACE  2ULL
#define PROCESS_CREATE_INLINE_LEN (SMALLSTRING_LEN_MAX)
#define PROCESS_CREATE_MSG     (         (((PROCESS_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (PROCESS_CREATE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_create_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    smallstring_t name;
} PACKED;
#define PROCESS_CREATE_REPLY_INLINE_LEN (sizeof(pid_t) + sizeof(cid_t))
#define PROCESS_CREATE_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (PROCESS_CREATE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_create_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
    cid_t pager_ch;
} PACKED;
#define PROCESS_DESTROY_INLINE_LEN (sizeof(pid_t))
#define PROCESS_DESTROY_MSG     (         (((PROCESS_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (PROCESS_DESTROY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_destroy_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
} PACKED;
#define PROCESS_DESTROY_REPLY_INLINE_LEN (0)
#define PROCESS_DESTROY_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (PROCESS_DESTROY_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_destroy_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define PROCESS_ADD_PAGER_INLINE_LEN (sizeof(pid_t) + sizeof(cid_t) + sizeof(uintptr_t) + sizeof(size_t) + sizeof(uint8_t))
#define PROCESS_ADD_PAGER_MSG     (         (((PROCESS_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (PROCESS_ADD_PAGER_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_add_pager_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
    cid_t pager;
    uintptr_t start;
    size_t size;
    uint8_t flags;
} PACKED;
#define PROCESS_ADD_PAGER_REPLY_INLINE_LEN (0)
#define PROCESS_ADD_PAGER_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| (PROCESS_ADD_PAGER_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_add_pager_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define PROCESS_SEND_CHANNEL_INLINE_LEN (sizeof(pid_t))
#define PROCESS_SEND_CHANNEL_MSG     (         (((PROCESS_INTERFACE << 8) | 4ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (PROCESS_SEND_CHANNEL_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_send_channel_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
} PACKED;
#define PROCESS_SEND_CHANNEL_REPLY_INLINE_LEN (0)
#define PROCESS_SEND_CHANNEL_REPLY_MSG     (         (((PROCESS_INTERFACE << 8) | MSG_REPLY_FLAG | 4) << MSG_TYPE_OFFSET)| (PROCESS_SEND_CHANNEL_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct process_send_channel_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define THREAD_INTERFACE  3ULL
#define THREAD_SPAWN_INLINE_LEN (sizeof(pid_t) + sizeof(uintptr_t) + sizeof(uintptr_t) + sizeof(uintptr_t) + sizeof(uintptr_t))
#define THREAD_SPAWN_MSG     (         (((THREAD_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (THREAD_SPAWN_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_spawn_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
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
    size_t num_pages;
    tid_t tid;
} PACKED;
#define THREAD_DESTROY_INLINE_LEN (sizeof(tid_t))
#define THREAD_DESTROY_MSG     (         (((THREAD_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (THREAD_DESTROY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_destroy_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    tid_t tid;
} PACKED;
#define THREAD_DESTROY_REPLY_INLINE_LEN (0)
#define THREAD_DESTROY_REPLY_MSG     (         (((THREAD_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (THREAD_DESTROY_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct thread_destroy_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define TIMER_INTERFACE  4ULL
typedef int32_t timer_id_t;
#define TIMER_SET_INLINE_LEN (sizeof(int32_t) + sizeof(int32_t))
#define TIMER_SET_MSG     (         (((TIMER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (TIMER_SET_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_set_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t initial;
    int32_t interval;
} PACKED;
#define TIMER_SET_REPLY_INLINE_LEN (sizeof(int32_t))
#define TIMER_SET_REPLY_MSG     (         (((TIMER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (TIMER_SET_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_set_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t timer;
} PACKED;
#define TIMER_CLEAR_INLINE_LEN (sizeof(int32_t))
#define TIMER_CLEAR_MSG     (         (((TIMER_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (TIMER_CLEAR_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_clear_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t timer;
} PACKED;
#define TIMER_CLEAR_REPLY_INLINE_LEN (0)
#define TIMER_CLEAR_REPLY_MSG     (         (((TIMER_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (TIMER_CLEAR_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct timer_clear_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define KERNEL_INTERFACE  9ULL
#define SERVER_INTERFACE  10ULL
#define SERVER_CONNECT_INLINE_LEN (sizeof(uint8_t))
#define SERVER_CONNECT_MSG     (         (((SERVER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (SERVER_CONNECT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct server_connect_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t interface;
} PACKED;
#define SERVER_CONNECT_REPLY_INLINE_LEN (0)
#define SERVER_CONNECT_REPLY_MSG     (         (((SERVER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (SERVER_CONNECT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct server_connect_reply_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define MEMORY_INTERFACE  11ULL
#define MEMORY_ALLOC_PAGES_INLINE_LEN (sizeof(size_t))
#define MEMORY_ALLOC_PAGES_MSG     (         (((MEMORY_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (MEMORY_ALLOC_PAGES_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_pages_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    size_t order;
} PACKED;
#define MEMORY_ALLOC_PAGES_REPLY_INLINE_LEN (0)
#define MEMORY_ALLOC_PAGES_REPLY_MSG     (         (((MEMORY_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (MEMORY_ALLOC_PAGES_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_pages_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t num_pages;
} PACKED;
#define MEMORY_ALLOC_PHY_PAGES_INLINE_LEN (sizeof(paddr_t) + sizeof(size_t))
#define MEMORY_ALLOC_PHY_PAGES_MSG     (         (((MEMORY_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (MEMORY_ALLOC_PHY_PAGES_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_phy_pages_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    paddr_t map_at;
    size_t order;
} PACKED;
#define MEMORY_ALLOC_PHY_PAGES_REPLY_INLINE_LEN (sizeof(paddr_t))
#define MEMORY_ALLOC_PHY_PAGES_REPLY_MSG     (         (((MEMORY_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (MEMORY_ALLOC_PHY_PAGES_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memory_alloc_phy_pages_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t num_pages;
    paddr_t paddr;
} PACKED;
#define PAGER_INTERFACE  12ULL
#define PAGER_FILL_INLINE_LEN (sizeof(pid_t) + sizeof(uintptr_t) + sizeof(size_t))
#define PAGER_FILL_MSG     (         (((PAGER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (PAGER_FILL_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pager_fill_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
    uintptr_t addr;
    size_t size;
} PACKED;
#define PAGER_FILL_REPLY_INLINE_LEN (0)
#define PAGER_FILL_REPLY_MSG     (         (((PAGER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (PAGER_FILL_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pager_fill_reply_payload {
    cid_t __unused_channel;
    vaddr_t page;
    size_t num_pages;
} PACKED;
#define MEMMGR_INTERFACE  13ULL
#define MEMMGR_GET_FRAMEBUFFER_INLINE_LEN (0)
#define MEMMGR_GET_FRAMEBUFFER_MSG     (         (((MEMMGR_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (MEMMGR_GET_FRAMEBUFFER_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memmgr_get_framebuffer_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define MEMMGR_GET_FRAMEBUFFER_REPLY_INLINE_LEN (sizeof(int32_t) + sizeof(int32_t) + sizeof(uint8_t))
#define MEMMGR_GET_FRAMEBUFFER_REPLY_MSG     (         (((MEMMGR_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (MEMMGR_GET_FRAMEBUFFER_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct memmgr_get_framebuffer_reply_payload {
    cid_t __unused_channel;
    vaddr_t framebuffer;
    size_t num_pages;
    int32_t width;
    int32_t height;
    uint8_t bpp;
} PACKED;
#define DISCOVERY_INTERFACE  14ULL
#define DISCOVERY_PUBLICIZE_INLINE_LEN (sizeof(uint8_t))
#define DISCOVERY_PUBLICIZE_MSG     (         (((DISCOVERY_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (DISCOVERY_PUBLICIZE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct discovery_publicize_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t interface;
} PACKED;
#define DISCOVERY_PUBLICIZE_REPLY_INLINE_LEN (0)
#define DISCOVERY_PUBLICIZE_REPLY_MSG     (         (((DISCOVERY_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (DISCOVERY_PUBLICIZE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct discovery_publicize_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define DISCOVERY_CONNECT_INLINE_LEN (sizeof(uint8_t))
#define DISCOVERY_CONNECT_MSG     (         (((DISCOVERY_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (DISCOVERY_CONNECT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct discovery_connect_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t interface;
} PACKED;
#define DISCOVERY_CONNECT_REPLY_INLINE_LEN (0)
#define DISCOVERY_CONNECT_REPLY_MSG     (         (((DISCOVERY_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (DISCOVERY_CONNECT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct discovery_connect_reply_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define IO_INTERFACE  15ULL
#define IO_ALLOW_IOMAPPED_IO_INLINE_LEN (0)
#define IO_ALLOW_IOMAPPED_IO_MSG     (         (((IO_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (IO_ALLOW_IOMAPPED_IO_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_allow_iomapped_io_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define IO_ALLOW_IOMAPPED_IO_REPLY_INLINE_LEN (0)
#define IO_ALLOW_IOMAPPED_IO_REPLY_MSG     (         (((IO_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (IO_ALLOW_IOMAPPED_IO_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_allow_iomapped_io_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define IO_LISTEN_IRQ_INLINE_LEN (sizeof(uint8_t))
#define IO_LISTEN_IRQ_MSG     (         (((IO_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (IO_LISTEN_IRQ_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_listen_irq_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t irq;
} PACKED;
#define IO_LISTEN_IRQ_REPLY_INLINE_LEN (0)
#define IO_LISTEN_IRQ_REPLY_MSG     (         (((IO_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (IO_LISTEN_IRQ_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct io_listen_irq_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define FS_INTERFACE  16ULL
typedef int32_t fd_t;
typedef uint8_t file_mode_t;
#define FS_OPEN_INLINE_LEN (SMALLSTRING_LEN_MAX + sizeof(uint8_t))
#define FS_OPEN_MSG     (         (((FS_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (FS_OPEN_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_open_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    smallstring_t path;
    uint8_t mode;
} PACKED;
#define FS_OPEN_REPLY_INLINE_LEN (sizeof(int32_t))
#define FS_OPEN_REPLY_MSG     (         (((FS_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (FS_OPEN_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_open_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t handle;
} PACKED;
#define FS_CLOSE_INLINE_LEN (sizeof(int32_t))
#define FS_CLOSE_MSG     (         (((FS_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (FS_CLOSE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_close_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t handle;
} PACKED;
#define FS_CLOSE_REPLY_INLINE_LEN (0)
#define FS_CLOSE_REPLY_MSG     (         (((FS_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (FS_CLOSE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_close_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define FS_READ_INLINE_LEN (sizeof(int32_t) + sizeof(size_t) + sizeof(size_t))
#define FS_READ_MSG     (         (((FS_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (FS_READ_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_read_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int32_t handle;
    size_t offset;
    size_t len;
} PACKED;
#define FS_READ_REPLY_INLINE_LEN (0)
#define FS_READ_REPLY_MSG     (         (((FS_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (FS_READ_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct fs_read_reply_payload {
    cid_t __unused_channel;
    vaddr_t data;
    size_t num_pages;
} PACKED;
#define GUI_INTERFACE  18ULL
#define GUI_CONSOLE_WRITE_INLINE_LEN (sizeof(uint8_t))
#define GUI_CONSOLE_WRITE_MSG     (         (((GUI_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (GUI_CONSOLE_WRITE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_console_write_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t ch;
} PACKED;
#define GUI_CONSOLE_WRITE_REPLY_INLINE_LEN (0)
#define GUI_CONSOLE_WRITE_REPLY_MSG     (         (((GUI_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (GUI_CONSOLE_WRITE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_console_write_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define GUI_ACTIVATE_INLINE_LEN (0)
#define GUI_ACTIVATE_MSG     (         (((GUI_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (GUI_ACTIVATE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_activate_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define GUI_ACTIVATE_REPLY_INLINE_LEN (0)
#define GUI_ACTIVATE_REPLY_MSG     (         (((GUI_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (GUI_ACTIVATE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_activate_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define GUI_KEY_EVENT_INLINE_LEN (sizeof(uint8_t))
#define GUI_KEY_EVENT_MSG     (         (((GUI_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (GUI_KEY_EVENT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_key_event_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t ch;
} PACKED;
#define GUI_KEY_EVENT_REPLY_INLINE_LEN (0)
#define GUI_KEY_EVENT_REPLY_MSG     (         (((GUI_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| (GUI_KEY_EVENT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct gui_key_event_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define BENCHMARK_INTERFACE  19ULL
#define BENCHMARK_NOP_INLINE_LEN (0)
#define BENCHMARK_NOP_MSG     (         (((BENCHMARK_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (BENCHMARK_NOP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct benchmark_nop_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define BENCHMARK_NOP_REPLY_INLINE_LEN (0)
#define BENCHMARK_NOP_REPLY_MSG     (         (((BENCHMARK_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (BENCHMARK_NOP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct benchmark_nop_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define API_INTERFACE  20ULL
#define API_CREATE_APP_INLINE_LEN (SMALLSTRING_LEN_MAX)
#define API_CREATE_APP_MSG     (         (((API_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (API_CREATE_APP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_create_app_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    smallstring_t path;
} PACKED;
#define API_CREATE_APP_REPLY_INLINE_LEN (sizeof(pid_t))
#define API_CREATE_APP_REPLY_MSG     (         (((API_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (API_CREATE_APP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_create_app_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
} PACKED;
#define API_START_APP_INLINE_LEN (sizeof(pid_t))
#define API_START_APP_MSG     (         (((API_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (API_START_APP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_start_app_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
} PACKED;
#define API_START_APP_REPLY_INLINE_LEN (0)
#define API_START_APP_REPLY_MSG     (         (((API_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (API_START_APP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_start_app_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define API_JOIN_APP_INLINE_LEN (sizeof(pid_t))
#define API_JOIN_APP_MSG     (         (((API_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (API_JOIN_APP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_join_app_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    pid_t pid;
} PACKED;
#define API_JOIN_APP_REPLY_INLINE_LEN (sizeof(int8_t))
#define API_JOIN_APP_REPLY_MSG     (         (((API_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| (API_JOIN_APP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_join_app_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int8_t code;
} PACKED;
#define API_EXIT_APP_INLINE_LEN (sizeof(int8_t))
#define API_EXIT_APP_MSG     (         (((API_INTERFACE << 8) | 4ULL) << MSG_TYPE_OFFSET)| (API_EXIT_APP_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_exit_app_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    int8_t code;
} PACKED;
#define API_EXIT_APP_REPLY_INLINE_LEN (0)
#define API_EXIT_APP_REPLY_MSG     (         (((API_INTERFACE << 8) | MSG_REPLY_FLAG | 4) << MSG_TYPE_OFFSET)| (API_EXIT_APP_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct api_exit_app_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define KEYBOARD_DRIVER_INTERFACE  30ULL
#define KEYBOARD_DRIVER_LISTEN_INLINE_LEN (0)
#define KEYBOARD_DRIVER_LISTEN_MSG     (         (((KEYBOARD_DRIVER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (KEYBOARD_DRIVER_LISTEN_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct keyboard_driver_listen_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define KEYBOARD_DRIVER_LISTEN_REPLY_INLINE_LEN (0)
#define KEYBOARD_DRIVER_LISTEN_REPLY_MSG     (         (((KEYBOARD_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (KEYBOARD_DRIVER_LISTEN_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct keyboard_driver_listen_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define KEYBOARD_DRIVER_KEYINPUT_EVENT_INLINE_LEN (sizeof(uint8_t))
#define KEYBOARD_DRIVER_KEYINPUT_EVENT_MSG     (         (((KEYBOARD_DRIVER_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (KEYBOARD_DRIVER_KEYINPUT_EVENT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct keyboard_driver_keyinput_event_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t keycode;
} PACKED;
#define KEYBOARD_DRIVER_KEYINPUT_EVENT_REPLY_INLINE_LEN (0)
#define KEYBOARD_DRIVER_KEYINPUT_EVENT_REPLY_MSG     (         (((KEYBOARD_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (KEYBOARD_DRIVER_KEYINPUT_EVENT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct keyboard_driver_keyinput_event_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define MOUSE_DRIVER_INTERFACE  31ULL
#define MOUSE_DRIVER_LISTEN_INLINE_LEN (0)
#define MOUSE_DRIVER_LISTEN_MSG     (         (((MOUSE_DRIVER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| MSG_CHANNEL_PAYLOAD| (MOUSE_DRIVER_LISTEN_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct mouse_driver_listen_payload {
    cid_t ch;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define MOUSE_DRIVER_LISTEN_REPLY_INLINE_LEN (0)
#define MOUSE_DRIVER_LISTEN_REPLY_MSG     (         (((MOUSE_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (MOUSE_DRIVER_LISTEN_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct mouse_driver_listen_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define MOUSE_DRIVER_MOUSE_EVENT_INLINE_LEN (sizeof(bool) + sizeof(bool) + sizeof(int16_t) + sizeof(int16_t))
#define MOUSE_DRIVER_MOUSE_EVENT_MSG     (         (((MOUSE_DRIVER_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (MOUSE_DRIVER_MOUSE_EVENT_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct mouse_driver_mouse_event_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    bool left_button;
    bool right_button;
    int16_t x;
    int16_t y;
} PACKED;
#define MOUSE_DRIVER_MOUSE_EVENT_REPLY_INLINE_LEN (0)
#define MOUSE_DRIVER_MOUSE_EVENT_REPLY_MSG     (         (((MOUSE_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (MOUSE_DRIVER_MOUSE_EVENT_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct mouse_driver_mouse_event_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define STORAGE_DRIVER_INTERFACE  32ULL
#define STORAGE_DRIVER_READ_INLINE_LEN (sizeof(uint64_t) + sizeof(size_t))
#define STORAGE_DRIVER_READ_MSG     (         (((STORAGE_DRIVER_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (STORAGE_DRIVER_READ_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct storage_driver_read_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint64_t sector;
    size_t len;
} PACKED;
#define STORAGE_DRIVER_READ_REPLY_INLINE_LEN (0)
#define STORAGE_DRIVER_READ_REPLY_MSG     (         (((STORAGE_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (STORAGE_DRIVER_READ_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct storage_driver_read_reply_payload {
    cid_t __unused_channel;
    vaddr_t data;
    size_t num_pages;
} PACKED;
#define STORAGE_DRIVER_WRITE_INLINE_LEN (sizeof(uint64_t) + sizeof(size_t))
#define STORAGE_DRIVER_WRITE_MSG     (         (((STORAGE_DRIVER_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| MSG_PAGE_PAYLOAD| (STORAGE_DRIVER_WRITE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct storage_driver_write_payload {
    cid_t __unused_channel;
    vaddr_t data;
    size_t num_pages;
    uint64_t sector;
    size_t len;
} PACKED;
#define STORAGE_DRIVER_WRITE_REPLY_INLINE_LEN (0)
#define STORAGE_DRIVER_WRITE_REPLY_MSG     (         (((STORAGE_DRIVER_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (STORAGE_DRIVER_WRITE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct storage_driver_write_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define PCI_INTERFACE  40ULL
#define PCI_WAIT_FOR_DEVICE_INLINE_LEN (sizeof(uint16_t) + sizeof(uint16_t))
#define PCI_WAIT_FOR_DEVICE_MSG     (         (((PCI_INTERFACE << 8) | 1ULL) << MSG_TYPE_OFFSET)| (PCI_WAIT_FOR_DEVICE_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_wait_for_device_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint16_t vendor;
    uint16_t device;
} PACKED;
#define PCI_WAIT_FOR_DEVICE_REPLY_INLINE_LEN (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t))
#define PCI_WAIT_FOR_DEVICE_REPLY_MSG     (         (((PCI_INTERFACE << 8) | MSG_REPLY_FLAG | 1) << MSG_TYPE_OFFSET)| (PCI_WAIT_FOR_DEVICE_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_wait_for_device_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint16_t subvendor;
    uint16_t subdevice;
    uint8_t bus;
    uint8_t slot;
} PACKED;
#define PCI_READ_CONFIG16_INLINE_LEN (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t))
#define PCI_READ_CONFIG16_MSG     (         (((PCI_INTERFACE << 8) | 2ULL) << MSG_TYPE_OFFSET)| (PCI_READ_CONFIG16_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_read_config16_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t bus;
    uint8_t slot;
    uint16_t offset;
} PACKED;
#define PCI_READ_CONFIG16_REPLY_INLINE_LEN (sizeof(uint16_t))
#define PCI_READ_CONFIG16_REPLY_MSG     (         (((PCI_INTERFACE << 8) | MSG_REPLY_FLAG | 2) << MSG_TYPE_OFFSET)| (PCI_READ_CONFIG16_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_read_config16_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint16_t data;
} PACKED;
#define PCI_READ_CONFIG32_INLINE_LEN (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t))
#define PCI_READ_CONFIG32_MSG     (         (((PCI_INTERFACE << 8) | 3ULL) << MSG_TYPE_OFFSET)| (PCI_READ_CONFIG32_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_read_config32_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t bus;
    uint8_t slot;
    uint16_t offset;
} PACKED;
#define PCI_READ_CONFIG32_REPLY_INLINE_LEN (sizeof(uint32_t))
#define PCI_READ_CONFIG32_REPLY_MSG     (         (((PCI_INTERFACE << 8) | MSG_REPLY_FLAG | 3) << MSG_TYPE_OFFSET)| (PCI_READ_CONFIG32_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_read_config32_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint32_t data;
} PACKED;
#define PCI_WRITE_CONFIG8_INLINE_LEN (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t))
#define PCI_WRITE_CONFIG8_MSG     (         (((PCI_INTERFACE << 8) | 4ULL) << MSG_TYPE_OFFSET)| (PCI_WRITE_CONFIG8_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_write_config8_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t bus;
    uint8_t slot;
    uint16_t offset;
    uint8_t data;
} PACKED;
#define PCI_WRITE_CONFIG8_REPLY_INLINE_LEN (0)
#define PCI_WRITE_CONFIG8_REPLY_MSG     (         (((PCI_INTERFACE << 8) | MSG_REPLY_FLAG | 4) << MSG_TYPE_OFFSET)| (PCI_WRITE_CONFIG8_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_write_config8_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;
#define PCI_WRITE_CONFIG32_INLINE_LEN (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t))
#define PCI_WRITE_CONFIG32_MSG     (         (((PCI_INTERFACE << 8) | 5ULL) << MSG_TYPE_OFFSET)| (PCI_WRITE_CONFIG32_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_write_config32_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
    uint8_t bus;
    uint8_t slot;
    uint16_t offset;
    uint32_t data;
} PACKED;
#define PCI_WRITE_CONFIG32_REPLY_INLINE_LEN (0)
#define PCI_WRITE_CONFIG32_REPLY_MSG     (         (((PCI_INTERFACE << 8) | MSG_REPLY_FLAG | 5) << MSG_TYPE_OFFSET)| (PCI_WRITE_CONFIG32_REPLY_INLINE_LEN << MSG_INLINE_LEN_OFFSET)     )

struct pci_write_config32_reply_payload {
    cid_t __unused_channel;
    vaddr_t __unused_page;
    size_t num_pages;
} PACKED;

#define IDL_MESSAGE_PAYLOADS \
    union { \
    struct runtime_exit_payload exit; \
    struct runtime_exit_reply_payload exit_reply; \
    struct runtime_printchar_payload printchar; \
    struct runtime_printchar_reply_payload printchar_reply; \
    } runtime; \
    union { \
    struct process_create_payload create; \
    struct process_create_reply_payload create_reply; \
    struct process_destroy_payload destroy; \
    struct process_destroy_reply_payload destroy_reply; \
    struct process_add_pager_payload add_pager; \
    struct process_add_pager_reply_payload add_pager_reply; \
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
    struct memory_alloc_pages_payload alloc_pages; \
    struct memory_alloc_pages_reply_payload alloc_pages_reply; \
    struct memory_alloc_phy_pages_payload alloc_phy_pages; \
    struct memory_alloc_phy_pages_reply_payload alloc_phy_pages_reply; \
    } memory; \
    union { \
    struct pager_fill_payload fill; \
    struct pager_fill_reply_payload fill_reply; \
    } pager; \
    union { \
    struct memmgr_get_framebuffer_payload get_framebuffer; \
    struct memmgr_get_framebuffer_reply_payload get_framebuffer_reply; \
    } memmgr; \
    union { \
    struct discovery_publicize_payload publicize; \
    struct discovery_publicize_reply_payload publicize_reply; \
    struct discovery_connect_payload connect; \
    struct discovery_connect_reply_payload connect_reply; \
    } discovery; \
    union { \
    struct io_allow_iomapped_io_payload allow_iomapped_io; \
    struct io_allow_iomapped_io_reply_payload allow_iomapped_io_reply; \
    struct io_listen_irq_payload listen_irq; \
    struct io_listen_irq_reply_payload listen_irq_reply; \
    } io; \
    union { \
    struct fs_open_payload open; \
    struct fs_open_reply_payload open_reply; \
    struct fs_close_payload close; \
    struct fs_close_reply_payload close_reply; \
    struct fs_read_payload read; \
    struct fs_read_reply_payload read_reply; \
    } fs; \
    union { \
    struct gui_console_write_payload console_write; \
    struct gui_console_write_reply_payload console_write_reply; \
    struct gui_activate_payload activate; \
    struct gui_activate_reply_payload activate_reply; \
    struct gui_key_event_payload key_event; \
    struct gui_key_event_reply_payload key_event_reply; \
    } gui; \
    union { \
    struct benchmark_nop_payload nop; \
    struct benchmark_nop_reply_payload nop_reply; \
    } benchmark; \
    union { \
    struct api_create_app_payload create_app; \
    struct api_create_app_reply_payload create_app_reply; \
    struct api_start_app_payload start_app; \
    struct api_start_app_reply_payload start_app_reply; \
    struct api_join_app_payload join_app; \
    struct api_join_app_reply_payload join_app_reply; \
    struct api_exit_app_payload exit_app; \
    struct api_exit_app_reply_payload exit_app_reply; \
    } api; \
    union { \
    struct keyboard_driver_listen_payload listen; \
    struct keyboard_driver_listen_reply_payload listen_reply; \
    struct keyboard_driver_keyinput_event_payload keyinput_event; \
    struct keyboard_driver_keyinput_event_reply_payload keyinput_event_reply; \
    } keyboard_driver; \
    union { \
    struct mouse_driver_listen_payload listen; \
    struct mouse_driver_listen_reply_payload listen_reply; \
    struct mouse_driver_mouse_event_payload mouse_event; \
    struct mouse_driver_mouse_event_reply_payload mouse_event_reply; \
    } mouse_driver; \
    union { \
    struct storage_driver_read_payload read; \
    struct storage_driver_read_reply_payload read_reply; \
    struct storage_driver_write_payload write; \
    struct storage_driver_write_reply_payload write_reply; \
    } storage_driver; \
    union { \
    struct pci_wait_for_device_payload wait_for_device; \
    struct pci_wait_for_device_reply_payload wait_for_device_reply; \
    struct pci_read_config16_payload read_config16; \
    struct pci_read_config16_reply_payload read_config16_reply; \
    struct pci_read_config32_payload read_config32; \
    struct pci_read_config32_reply_payload read_config32_reply; \
    struct pci_write_config8_payload write_config8; \
    struct pci_write_config8_reply_payload write_config8_reply; \
    struct pci_write_config32_payload write_config32; \
    struct pci_write_config32_reply_payload write_config32_reply; \
    } pci; \

struct message {
    header_t header;
    cid_t from;
    notification_t notification;
    union {
        // The standard layout of payloads.
        struct {
            cid_t channel;
            vaddr_t page;
            size_t num_pages;
            uint8_t data[INLINE_PAYLOAD_LEN_MAX];
        } PACKED;

        IDL_MESSAGE_PAYLOADS
    } payloads;
} PACKED;

STATIC_ASSERT(sizeof(struct message) == MSG_MAX_LEN);
STATIC_ASSERT(offsetof(struct message, header)              == 0);
STATIC_ASSERT(offsetof(struct message, from)                == 4);
STATIC_ASSERT(offsetof(struct message, notification)        == 8);
STATIC_ASSERT(offsetof(struct message, payloads.channel)    == 12);
STATIC_ASSERT(offsetof(struct message, payloads.page)       == 16);
STATIC_ASSERT(offsetof(struct message, payloads.num_pages)  == 24);
STATIC_ASSERT(offsetof(struct message, payloads.data)       == MSG_COMMON_HEADER_LEN);

//
//  Thread Information Block (TIB).
//

/// Thread Information Block (TIB).
struct thread_info {
    /// The pointer to itself in the userspace. This field must be the first.
    vaddr_t self;
    /// The user-provided argument.
    uintmax_t arg;
    /// The page base set by the user.
    vaddr_t page_addr;
    /// The maximum number of pages to receive page payloads.
    size_t num_pages;
    /// The thread-local user IPC buffer.
    struct message ipc_buffer;
} PACKED;

#endif
