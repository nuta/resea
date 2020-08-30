#include <list.h>
#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/timer.h>
#include <resea/handle.h>
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

static unsigned next_driver_id = 0;
static list_t drivers;
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
    struct message m;
    m.type = NET_TX_MSG;
    m.net_tx.payload = payload;
    m.net_tx.payload_len = len;
    async_send(driver->tid, &m);
}

static void deferred_work(void) {
    tcp_flush();

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
    struct message m;
    bzero(&m, sizeof(m));
    switch (e->type) {
        case TCP_NEW_CLIENT: {
            tcp_sock_t sock = e->tcp_new_client.listen_sock;
            m.type = TCPIP_NEW_CLIENT_MSG;
            m.tcpip_new_client.handle = sock->client->handle;
            async_send(sock->client->task, &m);
            break;
        }
        case TCP_RECEIVED: {
            tcp_sock_t sock = e->tcp_received.sock;
            if (!sock->client) {
                // Not yet accepted by the owner task.
                break;
            }

            m.type = TCPIP_RECEIVED_MSG;
            m.tcpip_received.handle = sock->client->handle;
            async_send(sock->client->task, &m);
            break;
        }
    }
}

msec_t sys_uptime(void) {
    return uptime;
}

static void free_handle(void *data) {
    struct client *c = data;
    tcp_close(c->sock);
}

void main(void) {
    TRACE("starting...");
    list_init(&drivers);

    // Initialize the TCP/IP protocol stack.
    device_init();
    tcp_init();
    udp_init();
    dhcp_init();

    error_t err = timer_set(TIMER_INTERVAL);
    ASSERT_OK(err);
    ASSERT_OK(ipc_serve("tcpip"));

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if ((m.notifications.data & NOTIFY_TIMER) != 0) {
                    error_t err = timer_set(TIMER_INTERVAL);
                    ASSERT_OK(err);
                    uptime += TIMER_INTERVAL;
                }

                if ((m.notifications.data & NOTIFY_ASYNC) != 0) {
                    struct message m;
                    async_recv(INIT_TASK, &m);
                    switch (m.type) {
                        case TASK_EXITED_MSG:
                            handle_free_all(m.task_exited.task, free_handle);
                            break;
                        default:
                           discard_unknown_message(&m);
                    }
                }

                break;
            case ASYNC_MSG:
                async_reply(m.src);
                break;
            case TCPIP_CONNECT_MSG: {
                tcp_sock_t sock = tcp_new();
                ipaddr_t dst_addr;
                dst_addr.type = IP_TYPE_V4;
                dst_addr.v4 = m.tcpip_connect.dst_addr;
                tcp_connect(sock, &dst_addr, m.tcpip_connect.dst_port);

                sock->client = malloc(sizeof(*sock->client));
                sock->client->sock = sock;
                sock->client->task = m.src;
                sock->client->handle = handle_alloc(m.src);
                handle_set(m.src, sock->client->handle, sock->client);

                m.type = TCPIP_CONNECT_REPLY_MSG;
                m.tcpip_connect_reply.handle = sock->client->handle;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_LISTEN_MSG: {
                tcp_sock_t sock = tcp_new();
                ipaddr_t any_ipaddr;
                any_ipaddr.type = IP_TYPE_V4;
                any_ipaddr.v4 = IPV4_ADDR_UNSPECIFIED;
                tcp_bind(sock, &any_ipaddr, m.tcpip_listen.port);
                tcp_listen(sock, m.tcpip_listen.backlog);

                sock->client = malloc(sizeof(*sock->client));
                sock->client->sock = sock;
                sock->client->task = m.src;
                sock->client->handle = handle_alloc(m.src);
                handle_set(m.src, sock->client->handle, sock->client);

                m.type = TCPIP_LISTEN_REPLY_MSG;
                m.tcpip_listen_reply.handle = sock->client->handle;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_ACCEPT_MSG: {
                struct client *c = handle_get(m.src, m.tcpip_accept.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_sock_t new_sock = tcp_accept(c->sock);
                if (!new_sock) {
                    ipc_send_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                new_sock->client = malloc(sizeof(*new_sock->client));
                new_sock->client->sock = new_sock;
                new_sock->client->task = m.src;
                new_sock->client->handle = handle_alloc(m.src);
                handle_set(m.src, new_sock->client->handle, new_sock->client);

                m.type = TCPIP_ACCEPT_REPLY_MSG;
                m.tcpip_accept_reply.new_handle = new_sock->client->handle;
                ipc_reply(m.src, &m);
                break;
            }
            case TCPIP_READ_MSG: {
                size_t max_len = MIN(4096, m.tcpip_read.len);
                struct client *c = handle_get(m.src, m.tcpip_read.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                m.type = TCPIP_READ_REPLY_MSG;
                uint8_t *buf = malloc(max_len);
                m.tcpip_read_reply.data = buf;
                m.tcpip_read_reply.data_len = tcp_read(c->sock, buf, max_len);
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            case TCPIP_WRITE_MSG: {
                struct client *c = handle_get(m.src, m.tcpip_write.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_write(c->sock, m.tcpip_write.data, m.tcpip_write.data_len);
                ipc_send_err(m.src, OK);
                free((void *) m.tcpip_write.data);
                break;
            }
            case TCPIP_CLOSE_MSG: {
                struct client *c = handle_get(m.src, m.tcpip_close.handle);
                if (!c) {
                    ipc_send_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_close(c->sock);
                break;
            }
            case TCPIP_REGISTER_DEVICE_MSG:
                register_device(m.src, &m.tcpip_register_device.macaddr);
                m.type = TCPIP_REGISTER_DEVICE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            case NET_RX_MSG: {
                struct driver *driver = get_driver_by_tid(m.src);
                if (!driver) {
                    break;
                }

                ethernet_receive(driver->device, m.net_rx.payload, m.net_rx.payload_len);
                free((void *) m.net_rx.payload);
                dhcp_receive();
                break;
            }
            default:
               discard_unknown_message(&m);
        }

        deferred_work();
    }
}
