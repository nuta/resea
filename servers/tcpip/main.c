#include <list.h>
#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/session.h>
#include <std/syscall.h>
#include "device.h"
#include "dhcp.h"
#include "main.h"
#include "sys.h"
#include "tcp.h"
#include "udp.h"

static unsigned next_driver_id = 0;
static list_t drivers;
static list_t pending_events;
static msec_t uptime = 0;

static struct driver *get_driver_by_tid(tid_t tid) {
    LIST_FOR_EACH (driver, &drivers, struct driver, next) {
        if (driver->tid == tid) {
            return driver;
        }
    }

    return NULL;
}

static void transmit(device_t device, mbuf_t pkt) {
    size_t len = mbuf_len(pkt);
    DEBUG_ASSERT(len <= 2048 && "too long packet");

    uint8_t *payload = malloc(len);
    mbuf_read(&pkt, payload, len);
    mbuf_delete(pkt);

    struct driver *driver = device->arg;
    struct packet *packet = malloc(sizeof(*packet));
    packet->dst = driver->tid;
    packet->m.type = NET_TX_MSG;
    packet->m.net_device.tx.payload = payload;
    packet->m.net_device.tx.len = len;
    list_push_back(&driver->tx_queue, &packet->next);

    OOPS_OK(ipc_notify(driver->tid, NOTIFY_NEW_DATA));
}

static error_t do_process_event(struct event *e) {
    // FIXME: Use ipc_notify
    switch (e->type) {
        case TCP_NEW_CLIENT: {
            tcp_sock_t sock = e->tcp_new_client.listen_sock;

            struct message m;
            m.type = TCPIP_NEW_CLIENT_MSG;
            m.tcpip.new_client.handle = sock->session->handle;
            error_t err = ipc_send_noblock(sock->session->owner, &m);
            ASSERT(err == OK || err == ERR_WOULD_BLOCK);

            if (err == ERR_WOULD_BLOCK) {
                ipc_listen(sock->session->owner);
            }

            return err;
        }
        case TCP_RECEIVED: {
            tcp_sock_t sock = e->tcp_received.sock;
            if (!sock->session) {
                // Not yet accepted by the owner task.
                WARN("not accepted");
                return ERR_NOT_FOUND;
            }

            struct message m;
            m.type = TCPIP_RECEIVED_MSG;
            m.tcpip.received.handle = sock->session->handle;
            error_t err = ipc_send_noblock(sock->session->owner, &m);
            ASSERT(err == OK || err == ERR_WOULD_BLOCK);

            if (err == ERR_WOULD_BLOCK) {
                WARN("woud block");
                ipc_listen(sock->session->owner);
            }

            return err;
        }
    }

    UNREACHABLE();
}

static void deferred_work(void) {
    tcp_flush();

    LIST_FOR_EACH (event, &pending_events, struct event, next) {
        error_t err = do_process_event(event);
        if (IS_OK(err)) {
            list_remove(&event->next);
            free(event);
        }
    }
}

static void register_device(tid_t driver_task, macaddr_t *macaddr) {
    if (next_driver_id > 9) {
        WARN("too many devices");
        return;
    }

    struct driver *driver = malloc(sizeof(*driver));
    next_driver_id++;
    driver->tid = driver_task;
    list_init(&driver->tx_queue);

    // Create a new device.
    char name[] = {'n', 'e', 't', '0' + next_driver_id, '\0'};
    device_t device = device_new(name, ethernet_transmit, transmit,
                                 (void *) (uintptr_t) driver);
    if (!device) {
        WARN("failed to create a device");
        free(driver);
        return;
    }

    list_push_back(&drivers, &driver->next);
    device_set_macaddr(device, macaddr);
    driver->device = device;

    device_enable_dhcp(device);
    INFO("registered new net device '%s'", name);
}

void sys_process_event(struct event *e) {
    error_t err = do_process_event(e);
    if (IS_ERROR(err)) {
        struct event *e_buf = malloc(sizeof(*e));
        memcpy(e_buf, e, sizeof(*e));
        list_push_back(&pending_events, &e_buf->next);
    }
}

msec_t sys_uptime(void) {
    return uptime;
}

void main(void) {
    INFO("starting...");
    list_init(&pending_events);
    list_init(&drivers);
    session_init();

    // Initialize the TCP/IP protocol stack.
    device_init();
    tcp_init();
    udp_init();
    dhcp_init();

    error_t err = timer_set(TIMER_INTERVAL);
    ASSERT_OK(err);

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if ((m.notifications.data & NOTIFY_READY) != 0) {
                    // Do nothing here: we just need to do deferred_work()
                    // below.
                }

                if ((m.notifications.data & NOTIFY_TIMER) != 0) {
                    error_t err = timer_set(TIMER_INTERVAL);
                    ASSERT_OK(err);
                    uptime += TIMER_INTERVAL;
                }
                break;
            case TCPIP_LISTEN_MSG: {
                tcp_sock_t sock = tcp_new();
                ipaddr_t any_ipaddr;
                any_ipaddr.type = IP_TYPE_V4;
                any_ipaddr.v4 = IPV4_ADDR_UNSPECIFIED;
                tcp_bind(sock, &any_ipaddr, m.tcpip.listen.port);
                tcp_listen(sock, m.tcpip.listen.backlog);

                sock->session = session_alloc(m.src);
                sock->session->data = sock;

                m.type = TCPIP_LISTEN_REPLY_MSG;
                m.tcpip.listen_reply.handle = sock->session->handle;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_ACCEPT_MSG: {
                struct session *sess =
                    session_get(m.src, m.tcpip.accept.handle);
                if (!sess) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_sock_t new_sock = tcp_accept(sess->data);
                if (!new_sock) {
                    ipc_send_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                new_sock->session = session_alloc(m.src);
                new_sock->session->data = new_sock;

                m.type = TCPIP_ACCEPT_REPLY_MSG;
                m.tcpip.accept_reply.new_handle = new_sock->session->handle;
                ipc_send(m.src, &m);
                break;
            }
            case TCPIP_READ_MSG: {
                size_t len = m.tcpip.read.len;
                struct session *sess = session_get(m.src, m.tcpip.read.handle);
                if (!sess) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                m.type = TCPIP_READ_REPLY_MSG;
                m.tcpip.read_reply.len =
                    tcp_read(sess->data, m.tcpip.read_reply.data,
                             MIN(len, TCP_DATA_LEN_MAX));
                ipc_send(m.src, &m);
                break;
            }
            case TCPIP_WRITE_MSG: {
                struct session *sess = session_get(m.src, m.tcpip.read.handle);
                if (!sess) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_write(sess->data, m.tcpip.write.data, m.tcpip.write.len);
                ipc_send_err(m.src, OK);
                break;
            }
            case TCPIP_REGISTER_DEVICE_MSG:
                register_device(m.src, &m.tcpip.register_device.macaddr);
                break;
            case NET_RX_MSG: {
                struct driver *driver = get_driver_by_tid(m.src);
                if (!driver) {
                    break;
                }

                ethernet_receive(driver->device, m.net_device.rx.payload, m.net_device.rx.len);
                dhcp_receive();
                break;
            }
            case NET_GET_TX_MSG: {
                struct driver *driver = get_driver_by_tid(m.src);
                if (!driver) {
                    break;
                }

                struct packet *pkt = LIST_POP_FRONT(&driver->tx_queue, struct packet, next);
                if (!pkt) {
                    ipc_reply_err(m.src, ERR_EMPTY);
                    break;
                }

                ipc_reply(m.src, &pkt->m);
                free(pkt->m.net_device.tx.payload);
                free(pkt);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
        }

        deferred_work();
    }
}
