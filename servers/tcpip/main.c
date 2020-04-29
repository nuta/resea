#include <list.h>
#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/map.h>
#include <std/syscall.h>
#include <std/rand.h>
#include "device.h"
#include "dhcp.h"
#include "main.h"
#include "sys.h"
#include "tcp.h"
#include "udp.h"

struct client {
    task_t task;
    handle_t handle;
    tcp_sock_t sock;
};

struct pending {
    list_elem_t next;
    struct message m;
};

static map_t clients;
static unsigned next_driver_id = 0;
static list_t drivers;
static list_t pending_events;
static list_t pending_msgs;
static msec_t uptime = 0;

static struct driver *get_driver_by_tid(task_t tid) {
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
    packet->m.net_tx.payload = payload;
    packet->m.net_tx.len = len;
    list_push_back(&driver->tx_queue, &packet->next);

    OOPS_OK(ipc_notify(driver->tid, NOTIFY_NEW_DATA));
}

static error_t do_process_event(struct event *e) {
    struct pending *pending = malloc(sizeof(*pending));
    switch (e->type) {
        case TCP_NEW_CLIENT: {
            tcp_sock_t sock = e->tcp_new_client.listen_sock;

            pending->m.type = TCPIP_NEW_CLIENT_MSG;
            pending->m.tcpip_new_client.handle = sock->client->handle;
            ipc_notify(sock->client->task, NOTIFY_NEW_DATA);
            break;
        }
        case TCP_RECEIVED: {
            tcp_sock_t sock = e->tcp_received.sock;
            if (!sock->client) {
                // Not yet accepted by the owner task.
                free(pending);
                return ERR_NOT_FOUND;
            }

            pending->m.type = TCPIP_RECEIVED_MSG;
            pending->m.tcpip_received.handle = sock->client->handle;
            ipc_notify(sock->client->task, NOTIFY_NEW_DATA);
        }
    }

    // TODO: Use client-local queues.
    list_push_back(&pending_msgs, &pending->next);
    return OK;
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

    // TODO:
    LIST_FOR_EACH(driver, &drivers, struct driver, next) {
        if (!driver->device->dhcp_enabled
            && driver->dhcp_discover_retires < 10
            && driver->last_dhcp_discover + 200 < sys_uptime()) {
            WARN("retrying DHCP discover...");
            dhcp_transmit(driver->device, DHCP_TYPE_DISCOVER, IPV4_ADDR_UNSPECIFIED);
            driver->last_dhcp_discover = sys_uptime();
            driver->dhcp_discover_retires++;
        }
    }
}

static void register_device(task_t driver_task, macaddr_t *macaddr) {
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
    driver->dhcp_discover_retires = 0;
    driver->last_dhcp_discover = sys_uptime();

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
    list_init(&pending_msgs);
    list_init(&drivers);
    clients = map_new();

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
                tcp_bind(sock, &any_ipaddr, m.tcpip_listen.port);
                tcp_listen(sock, m.tcpip_listen.backlog);

                handle_t handle;
                rand_bytes((uint8_t *) &handle, sizeof(handle));

                sock->client = malloc(sizeof(*sock->client));
                sock->client->sock = sock;
                sock->client->task = m.src;
                sock->client->handle = handle;
                map_set_handle(clients, &handle, sock->client);

                m.type = TCPIP_LISTEN_REPLY_MSG;
                m.tcpip_listen_reply.handle = sock->client->handle;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_ACCEPT_MSG: {
                struct client *c =
                    map_get_handle(clients, &m.tcpip_accept.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_sock_t new_sock = tcp_accept(c->sock);
                if (!new_sock) {
                    ipc_send_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                handle_t new_handle;
                rand_bytes((uint8_t *) &new_handle, sizeof(new_handle));

                new_sock->client = malloc(sizeof(*new_sock->client));
                new_sock->client->sock = new_sock;
                new_sock->client->task = m.src;
                new_sock->client->handle = new_handle;
                map_set_handle(clients, &new_handle, new_sock->client);

                m.type = TCPIP_ACCEPT_REPLY_MSG;
                m.tcpip_accept_reply.new_handle = new_sock->client->handle;
                ipc_send(m.src, &m);
                break;
            }
            case TCPIP_READ_MSG: {
                size_t max_len = MIN(4096, m.tcpip_read.len);
                struct client *c =
                    map_get_handle(clients, &m.tcpip_read.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                m.type = TCPIP_READ_REPLY_MSG;
                uint8_t *buf = malloc(max_len);
                m.tcpip_read_reply.data = buf;
                m.tcpip_read_reply.len = tcp_read(c->sock, buf, max_len);
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            case TCPIP_WRITE_MSG: {
                struct client *c =
                    map_get_handle(clients, &m.tcpip_write.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_write(c->sock, m.tcpip_write.data, m.tcpip_write.len);
                ipc_send_err(m.src, OK);
                free(m.tcpip_write.data);
                break;
            }
            case TCPIP_REGISTER_DEVICE_MSG:
                register_device(m.src, &m.tcpip_register_device.macaddr);
                break;
            case TCPIP_PULL_MSG: {
                struct pending *pending =
                    LIST_POP_FRONT(&pending_msgs, struct pending, next);
                if (!pending) {
                    ipc_reply_err(m.src, ERR_EMPTY);
                }

                ipc_reply(m.src, &pending->m);
                free(pending);
                break;
            }
            case NET_RX_MSG: {
                struct driver *driver = get_driver_by_tid(m.src);
                if (!driver) {
                    break;
                }

                ethernet_receive(driver->device, m.net_rx.payload, m.net_rx.len);
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
                free(pkt->m.net_tx.payload);
                free(pkt);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
        }

        deferred_work();
    }
}
