#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <types.h>

typedef int handle_t;

//
//  Network Device Driver
//

#define NET_PACKET_LEN_MAX 1000

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
//  TCP/IP Server
//
#define TCP_DATA_LEN_MAX 1000

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

    NET_RX_MSG,
    NET_GET_TX_MSG,

    SCREEN_DRAW_CHAR_MSG,
    SCREEN_MOVE_CURSOR_MSG,
    SCREEN_CLEAR_MSG,
    SCREEN_SCROLL_MSG,
    SCREEN_GET_SIZE_MSG,
    SCREEN_GET_SIZE_REPLY_MSG,

    KBD_KEYCODE_MSG,
    KBD_GET_KEYCODE_MSG,

    TCPIP_REGISTER_DEVICE_MSG,
    TCPIP_LISTEN_MSG,
    TCPIP_LISTEN_REPLY_MSG,
    TCPIP_ACCEPT_MSG,
    TCPIP_ACCEPT_REPLY_MSG,
    TCPIP_WRITE_MSG,
    TCPIP_READ_MSG,
    TCPIP_READ_REPLY_MSG,
    TCPIP_PULL_MSG,
    TCPIP_NEW_CLIENT_MSG,
    TCPIP_CLOSE_MSG,
    TCPIP_CLOSED_MSG,
    TCPIP_RECEIVED_MSG,
};

/// Message.
struct message {
    int type;
    tid_t src;
    union {
        // Kernel Messages
        union {
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
        };

        // Memory Manager (memmgr)
        union {
           struct {
               char name[64];
           } lookup;
    
           struct {
               tid_t task;
           } lookup_reply;
    
           struct {
           } nop;
 
             struct {
                size_t num_pages;
                paddr_t paddr;
            } alloc_pages;

            struct {
                vaddr_t vaddr;
                paddr_t paddr;
            } alloc_pages_reply;
        };

        // TCP/IP
        union {
            struct {
                uint8_t macaddr[6];
            } register_device;

            struct {
                uint16_t port;
                int backlog;
            } listen;

            struct {
                handle_t handle;
            } listen_reply;

            struct {
                handle_t handle;
            } closed;

            struct {
                handle_t handle;
            } close;

            struct {
                handle_t handle;
            } new_client;

            struct {
                handle_t handle;
                size_t len;
                uint8_t data[TCP_DATA_LEN_MAX];
            } write;

            struct {
                handle_t handle;
            } accept;

            struct {
                handle_t new_handle;
            } accept_reply;

            struct {
                handle_t handle;
                size_t len;
            } read;

            struct {
                size_t len;
                uint8_t data[TCP_DATA_LEN_MAX];
            } read_reply;

            struct {
                handle_t handle;
            } received;
        } tcpip;

        // Shell
        union {
            struct {
                uint16_t keycode;
            } key_pressed;
        } shell;

        // Network Device Driver
        union {
            struct {
                size_t len;
                uint8_t *payload;
            } tx;

            struct {
                size_t len;
                uint8_t payload[NET_PACKET_LEN_MAX];
            } rx;
        } net_device;

        // Screen Device Driver
        union {
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
        } screen_device;
   };
};

#define DEFINE_MSG __COUNTER__
#define _NTH_MEMBER(member) \
    ({ \
        STATIC_ASSERT(offsetof(struct message, member) % sizeof(uintptr_t) == 0); \
        STATIC_ASSERT(offsetof(struct message, member) / sizeof(uintptr_t) <= 7); \
        (int) (offsetof(struct message, member) / sizeof(uintptr_t)); \
    })

#define DEFINE_MSG_WITH_BULK(bulk_ptr, bulk_len)                               \
    (DEFINE_MSG | MSG_BULK(_NTH_MEMBER(bulk_ptr), _NTH_MEMBER(bulk_len)))

#define NET_TX_MSG DEFINE_MSG_WITH_BULK(net_device.tx.payload, net_device.tx.len)

// Ensure that a message is not too big.
STATIC_ASSERT(sizeof(struct message) <= 1024);
// Ensure that bulk ptr/len offsets cannot point beyond the message.
STATIC_ASSERT(0x7 * sizeof(uintptr_t) < sizeof(struct message) - sizeof(uintptr_t));

#endif
