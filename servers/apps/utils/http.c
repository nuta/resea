#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/ctype.h>
#include <string.h>

static task_t tcpip_server;

static void send(handle_t handle, const uint8_t *buf, size_t len) {
    struct message m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.handle = handle;
    m.tcpip_write.data = buf;
    m.tcpip_write.data_len = len;
    ASSERT_OK(ipc_call(tcpip_server, &m));
}

static void received(handle_t handle, uint8_t *buf, size_t len) {
    char *sep = strstr((char *) buf, "\r\n\r\n");
    if (!sep) {
        WARN("failed to find \\r\\n\\r\\n");
        return;
    }

    char *body = sep + 4; /* skip \r\n\r\n */
    DBG("%s", body);

    struct message m;
    m.type = TCPIP_CLOSE_MSG;
    m.tcpip_close.handle = handle;
    ASSERT_OK(ipc_call(tcpip_server, &m));
}

static error_t parse_ipaddr(const char *str, uint32_t *ip_addr) {
    char *s_orig = strdup(str);
    char *s = s_orig;
    for (int i = 0; i < 4; i++) {
        char *part = s;
        s = strchr(s, '.');
        if (!s) {
            free(s_orig);
            return ERR_INVALID_ARG;
        }

        *s++ = '\0';
        *ip_addr = (*ip_addr << 8) | atoi(part);
    }

    free(s_orig);
    return OK;
}

static void resolve_url(const char *url, uint32_t *ip_addr, uint16_t *port, char **path) {
    char *s_orig = strdup(url);
    char *s = s_orig;
    if (strstr(s, "http://") == s) {
        s += 7; // strlen("http://")
    } else {
        PANIC("the url must start with http://");
    }

    // `s` now points to the beginning of hostname or IP address.
    char *host = s;
    char *sep = strchr(s, ':');
    if (*sep == '\0') {
        // The port number is not given.
        *port = 80;
        s = strchr(s, '/');
        DBG("s = '%s'", s);
        if (*s) {
            *s++ = '\0';
        }
    } else {
        *sep = '\0';
        char *port_str = sep + 1; // Next to ':'.
        s = strchr(port_str, '/');
        *s++ = '\0';
        *port = atoi(port_str);
    }

    if (!isdigit(*host)) {
        PANIC("resolving a domain name is not yet implemented");
    }

    // `s` now points to the path next to the first slash.
    if (parse_ipaddr(host, ip_addr) != OK) {
        PANIC("failed to parse an ip address: '%s'", host);
    }

    *path = s;
    free(s_orig);
}

void http_get(const char *url) {
    tcpip_server = ipc_lookup("tcpip");

    uint32_t ip_addr;
    uint16_t port;
    char *path;
    resolve_url(url, &ip_addr, &port, &path);

    struct message m;
    m.type = TCPIP_CONNECT_MSG;
    m.tcpip_connect.dst_addr = ip_addr;
    m.tcpip_connect.dst_port = port;
    ASSERT_OK(ipc_call(tcpip_server, &m));
    handle_t handle = m.tcpip_connect_reply.handle;

    int buf_len = 1024;
    char *buf = malloc(buf_len);
    DBG("path = '%s'", path);
    snprintf(buf, buf_len, "GET /%s HTTP/1.0\r\n\r\n", path);
    send(handle, (uint8_t *) buf, strlen(buf));
    free(buf);

    INFO("ready");
    while (1) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG: {
                if (m.notifications.data & NOTIFY_ASYNC) {
                    ASSERT_OK(async_recv(tcpip_server, &m));
                    switch (m.type) {
                        case TCPIP_RECEIVED_MSG: {
                            m.type = TCPIP_READ_MSG;
                            m.tcpip_read.handle = m.tcpip_received.handle;
                            m.tcpip_read.len = 4096;
                            ASSERT_OK(ipc_call(tcpip_server, &m));

                            uint8_t *buf = (uint8_t *) m.tcpip_read_reply.data;
                            size_t len = m.tcpip_read_reply.data_len;
                            received(handle, buf, len);
                            free(buf);
                            break;
                        }
                        default:
                        WARN("unknown async message type (type=%d)", m.type);
                    }
                }
                break;
            };
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
