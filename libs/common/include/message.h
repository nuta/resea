#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <types.h>

typedef struct {
    uint8_t id[16];
} handle_t;

//
//  Console Device Driver
//

/// Screen text color codes. See https://wiki.osdev.org/Printing_To_Screen
typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 9,
    COLOR_GREEN = 10,
    COLOR_CYAN = 11,
    COLOR_RED = 12,
    COLOR_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15,
    COLOR_NORMAL = 15,
} color_t;

//
//  Keyboard Device Driver
//

/// Lower 7 bits are ASCII code and upper bits are modifier keys combination
/// such as Ctrl and Alt.
typedef uint16_t keycode_t;

#define KEY_MOD_CTRL (1 << 8)
#define KEY_MOD_ALT  (1 << 9)

#define ID(x)  (x)
#define _NTH_MEMBER(member) \
    ({ \
        STATIC_ASSERT(offsetof(struct message, member) % sizeof(uintptr_t) == 0); \
        STATIC_ASSERT(offsetof(struct message, member) / sizeof(uintptr_t) <= 7); \
        (int) (offsetof(struct message, member) / sizeof(uintptr_t)); \
    })
#define BULK(bulk_ptr, bulk_len)                               \
    (MSG_BULK(_NTH_MEMBER(bulk_ptr), _NTH_MEMBER(bulk_len)))

/// Message.
struct message {
    int type;
    task_t src;
    union {
        uint8_t raw[64 - sizeof(int) - sizeof(task_t)];

        #define NOTIFICATIONS_MSG ID(1)
        struct {
            notifications_t data;
        } notifications;

        #define EXCEPTION_MSG ID(2)
        struct {
            task_t task;
            enum exception_type exception;
        } exception;

        #define PAGE_FAULT_MSG ID(3)
        struct {
            task_t task;
            vaddr_t vaddr;
            pagefault_t fault;
        } page_fault;

        #define PAGE_FAULT_REPLY_MSG ID(4)
        struct {
            paddr_t paddr;
            pageattrs_t attrs;
        } page_fault_reply;

        #define LOOKUP_MSG ID(5)
        struct {
            char name[32];
        } lookup;

        #define LOOKUP_REPLY_MSG ID(6)
        struct {
            task_t task;
        } lookup_reply;

        #define NOP_MSG ID(10)
        struct {
        } nop;

        #define ALLOC_PAGES_MSG ID(11)
        struct {
            size_t num_pages;
            paddr_t paddr;
        } alloc_pages;

        #define ALLOC_PAGES_REPLY_MSG ID(12)
        struct {
            vaddr_t vaddr;
            paddr_t paddr;
        } alloc_pages_reply;

        #define EXEC_MSG ID(13)
        struct {
            char name[16];
            task_t server;
            handle_t handle;
        } exec;

        #define EXEC_REPLY_MSG ID(14)
        struct {
            task_t task;
        } exec_reply;

        #define JOIN_MSG ID(15)
        struct {
            task_t task;
        } join;

        #define JOIN_REPLY_MSG ID(16)
        struct {
        } join_reply;

        #define FS_OPEN_MSG (ID(50) | BULK(fs_open.path, fs_open.len))
        struct {
            char *path;
            size_t len;
        } fs_open;

        #define FS_OPEN_REPLY_MSG ID(51)
        struct {
            handle_t handle;
        } fs_open_reply;

        #define FS_CLOSE_MSG ID(52)
        struct {
            handle_t handle;
        } fs_close;

        #define FS_CLOSE_REPLY_MSG ID(53)
        struct {
        } fs_close_reply;

        #define FS_READ_MSG ID(54)
        struct {
            handle_t handle;
            offset_t offset;
            size_t len;
        } fs_read;

        #define FS_READ_REPLY_MSG (ID(55 | BULK(fs_read_reply.data, fs_read_reply.len)))
        struct {
            void *data;
            size_t len;
        } fs_read_reply;

        #define TCPIP_REGISTER_DEVICE_MSG ID(70)
        struct {
            uint8_t macaddr[6];
        } tcpip_register_device;

        #define TCPIP_LISTEN_MSG ID(71)
        struct {
            uint16_t port;
            int backlog;
        } tcpip_listen;

        #define TCPIP_LISTEN_REPLY_MSG ID(72)
        struct {
            handle_t handle;
        } tcpip_listen_reply;

        #define TCPIP_CLOSED_MSG ID(73)
        struct {
            handle_t handle;
        } tcpip_closed;

        #define TCPIP_CLOSE_MSG ID(74)
        struct {
            handle_t handle;
        } tcpip_close;

        #define TCPIP_NEW_CLIENT_MSG ID(75)
        struct {
            handle_t handle;
        } tcpip_new_client;

        #define TCPIP_WRITE_MSG (ID(76) | BULK(tcpip_write.data, tcpip_write.len))
        struct {
            void *data;
            size_t len;
            handle_t handle;
        } tcpip_write;

        #define TCPIP_ACCEPT_MSG ID(78)
        struct {
            handle_t handle;
        } tcpip_accept;

        #define TCPIP_ACCEPT_REPLY_MSG ID(79)
        struct {
            handle_t new_handle;
        } tcpip_accept_reply;

        #define TCPIP_READ_MSG ID(80)
        struct {
            handle_t handle;
            size_t len;
        } tcpip_read;

        #define TCPIP_READ_REPLY_MSG (ID(81) | BULK(tcpip_read_reply.data, tcpip_read_reply.len))
        struct {
            void *data;
            size_t len;
        } tcpip_read_reply;

        // FIXME:
        #define TCPIP_PULL_MSG ID(82)
        #define TCPIP_RECEIVED_MSG ID(83)
        struct {
            handle_t handle;
        } tcpip_received;

        // FIXME:
        #define NET_GET_TX_MSG ID(100)
        #define NET_TX_MSG (ID(101) | BULK(net_tx.payload, net_tx.len))
        struct {
            void *payload;
            size_t len;
        } net_tx;

        #define NET_RX_MSG (ID(102) | BULK(net_rx.payload, net_rx.len))
        struct {
            void *payload;
            size_t len;
        } net_rx;

        #define BLK_READ_MSG ID(120)
        struct {
            offset_t sector;
            size_t num_sectors;
        } blk_read;

        #define BLK_READ_REPLY_MSG (ID(121) | BULK(blk_read_reply.data, blk_read_reply.len))
        struct {
            uint8_t *data;
            size_t len;
        } blk_read_reply;

        // FIXME:
        #define KBD_GET_KEYCODE_MSG ID(110)
        #define KBD_KEYCODE_MSG ID(110)
        struct {
            uint16_t keycode;
        } key_pressed;

        #define TEXTSCREEN_DRAW_CHAR_MSG ID(150)
        struct {
            char ch;
            color_t fg_color;
            color_t bg_color;
            unsigned x;
            unsigned y;
        } textscreen_draw_char;

        #define TEXTSCREEN_MOVE_CURSOR_MSG ID(151)
        struct {
            unsigned x;
            unsigned y;
        } textscreen_move_cursor;

        #define TEXTSCREEN_CLEAR_MSG ID(152)
        struct {
        } textscreen_clear;

        #define TEXTSCREEN_SCROLL_MSG ID(153)
        struct {
        } textscreen_scroll;

        #define TEXTSCREEN_GET_SIZE_MSG ID(154)
        struct {
        } textscreen_get_size;

        #define TEXTSCREEN_GET_SIZE_REPLY_MSG ID(155)
        struct {
            unsigned width;
            unsigned height;
        } textscreen_get_size_reply;
   };
};

STATIC_ASSERT(sizeof(struct message) == 64);

#endif
