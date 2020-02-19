#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <types.h>

typedef int handle_t;

//
//  Network Device Driver
//

#define NET_PACKET_LEN_MAX 1024

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

//
//  KVS Server
//
#define KVS_KEY_LEN      64
#define KVS_DATA_LEN_MAX 1024

//
//  TCP/IP Server
//
#define TCP_DATA_LEN_MAX 1024

/// Message Types.
enum message_type {
    /// An unused type. If you've received this message, there must be a bug!
    NULL_MSG,

    NOTIFICATIONS_MSG,
    EXCEPTION_MSG,
    PAGE_FAULT_MSG,
    PAGE_FAULT_REPLY_MSG,

    NOP_MSG,
    LOOKUP_MSG,
    LOOKUP_REPLY_MSG,

    ALLOC_PAGES_MSG,
    ALLOC_PAGES_REPLY_MSG,

    NET_TX_MSG,
    NET_RX_MSG,

    KBD_ON_PRESSED_MSG,

    DRAW_CHAR_MSG,
    MOVE_CURSOR_MSG,
    CLEAR_DISPLAY_MSG,
    SCROLL_DISPLAY_MSG,
    DISPLAY_GET_SIZE_MSG,
    DISPLAY_GET_SIZE_REPLY_MSG,

    TCPIP_REGISTER_DEVICE_MSG,
    TCP_LISTEN_MSG,
    TCP_LISTEN_REPLY_MSG,
    TCP_ACCEPT_MSG,
    TCP_ACCEPT_REPLY_MSG,
    TCP_WRITE_MSG,
    TCP_READ_MSG,
    TCP_READ_REPLY_MSG,
    TCP_NEW_CLIENT_MSG,
    TCP_CLOSE_MSG,
    TCP_CLOSED_MSG,
    TCP_RECEIVED_MSG,

    KVS_GET_MSG,
    KVS_GET_REPLY_MSG,
    KVS_SET_MSG,
    KVS_DELETE_MSG,
    KVS_LISTEN_MSG,
    KVS_UNLISTEN_MSG,
    KVS_CHANGED_MSG,
};

/// Message.
struct message {
    enum message_type type;
    tid_t src;
    size_t len;
    union {
        // KVS
        union {
            struct {
                size_t len;
                char key[KVS_KEY_LEN];
            } get;

            struct {
                size_t len;
                uint8_t data[];
            } get_reply;

            struct {
                size_t len;
                char key[KVS_KEY_LEN];
                uint8_t data[];
            } set;

            struct {
                char key[KVS_KEY_LEN];
            } delete;

            struct {
                char key[KVS_KEY_LEN];
            } listen;

            struct {
                char key[KVS_KEY_LEN];
            } unlisten;

            struct {
                char key[KVS_KEY_LEN];
            } changed;
        } kvs;

        struct {
            uint8_t macaddr[6];
        } tcpip_register_device;

        struct {
            uint16_t port;
            int backlog;
        } tcp_listen;

        struct {
            handle_t handle;
        } tcp_listen_reply;

        struct {
            handle_t handle;
        } tcp_closed;

        struct {
            handle_t handle;
        } tcp_close;

        struct {
            handle_t handle;
        } tcp_new_client;

        struct {
            handle_t handle;
            size_t len;
            uint8_t data[];
        } tcp_write;

        struct {
            handle_t handle;
        } tcp_accept;

        struct {
            handle_t new_handle;
        } tcp_accept_reply;

        struct {
            handle_t handle;
            size_t len;
        } tcp_read;

        struct {
            size_t len;
            uint8_t data[];
        } tcp_read_reply;

        struct {
            handle_t handle;
        } tcp_received;

        struct {
            notifications_t data;
        } notifications;

        struct {
            tid_t task;
            enum exception_type exception;
        } exception;

        struct {
            tid_t task;
            vaddr_t vaddr;
            pagefault_t fault;
        } page_fault;

        struct {
            paddr_t paddr;
            pageattrs_t attrs;
        } page_fault_reply;

        struct {
            size_t num_pages;
            paddr_t paddr;
        } alloc_pages;

        struct {
            vaddr_t vaddr;
            paddr_t paddr;
        } alloc_pages_reply;

        struct {
            size_t len;
            uint8_t payload[];
        } net_tx;

        struct {
            size_t len;
            uint8_t payload[];
        } net_rx;

        struct {
            uint16_t keycode;
        } kbd_on_pressed;

        struct {
            char ch;
            color_t fg_color;
            color_t bg_color;
            unsigned x;
            unsigned y;
        } draw_char;

        struct {
            unsigned x;
            unsigned y;
        } move_cursor;

        struct {
        } clear_display;

        struct {
        } scroll_display;

        struct {
        } display_get_size;

        struct {
            unsigned width;
            unsigned height;
        } display_get_size_reply;

        struct {
            char name[64];
        } lookup;

        struct {
            tid_t task;
        } lookup_reply;

        struct {
        } nop;
    };
};

// Ensure that a message is not too big.
STATIC_ASSERT(sizeof(struct message) <= 128);

/// The maximum size of a message. Note that a message can be larger than
/// `sizeof(struct message)`.
#define MESSAGE_MAX_SIZE 2047

#endif
