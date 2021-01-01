#include "webapi.h"
#include <resea/async.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <vprintf.h>

static list_t clients;
static task_t tcpip_server;

#define INDEX_HTML                                                             \
    "<!DOCTYPE html>"                                                          \
    "<head>"                                                                   \
    "<html>"                                                                   \
    "   <meta charset=\"utf-8\">"                                              \
    "</head>"                                                                  \
    "<body>"                                                                   \
    "<h1>Hello from WebAPI server!</h1>"                                       \
    "</body>"                                                                  \
    "</html>"

/// Sends a HTTP response. This function assumes `payload` is a null-terminated
/// string.
static void write_response(struct client *client, const char *status,
                           const char *payload) {
    int buf_size = 1024;
    int off = 0;
    char *buf = malloc(buf_size);

    off += snprintf(&buf[off], buf_size - off, "HTTP/1.1 %s\r\n", status);
    off += snprintf(&buf[off], buf_size - off, "Connection: close\r\n");
    off += snprintf(&buf[off], buf_size - off, "Content-Length: %d\r\n",
                    strlen(payload));
    off += snprintf(&buf[off], buf_size - off, "\r\n");
    off += snprintf(&buf[off], buf_size - off, "%s", payload);
    if (buf_size - off <= 0) {
        WARN("too long HTTP response, aborting...");
        return;
    }

    struct message m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.handle = client->handle;
    m.tcpip_write.data = buf;
    m.tcpip_write.data_len = strlen(buf);
    ASSERT_OK(ipc_call(tcpip_server, &m));
    free(buf);
}

/// Handles a HTTP request.
static void process_request(struct client *client, const char *method,
                            const char *path) {
    TRACE("%s %s", method, path);
    bool get = !strcmp(method, "GET");
    if (get && !strcmp(path, "/")) {
        write_response(client, "200 OK", INDEX_HTML);
    } else {
        write_response(client, "404 Not Found", "");
    }
}

/// Handles incoming data.
static void process(struct client *client, uint8_t *data, size_t len) {
    if (client->done) {
        return;
    }

    // Copy the received payload.
    size_t new_len = client->request_len + len + 1;  // including '\0'
    client->request = realloc(client->request, new_len);
    memcpy(&client->request[client->request_len], data, len);
    client->request_len = new_len;

    // Return if we have not yet received the complete HTTP request.
    if (!strstr(client->request, "\r\n\r\n")) {
        return;
    }

    // Now we've received the complete HTTP request. Parse it and send a
    // HTTP response...
    client->request[client->request_len] = '\0';
    char *method = client->request;
    char *method_end = strstr(method, " ");
    if (!method_end) {
        goto malformed;
    }

    char *path = method_end + 1;
    char *path_end = strstr(path, " ");
    if (!path_end) {
        goto malformed;
    }

    *method_end = '\0';
    *path_end = '\0';
    process_request(client, method, path);

    client->done = true;
    return;

    struct message m;
malformed:
    m.type = TCPIP_CLOSE_MSG;
    m.tcpip_close.handle = client->handle;
    ipc_call(tcpip_server, &m);
}

static void tcp_read(handle_t handle) {
    struct client *client = NULL;
    LIST_FOR_EACH (c, &clients, struct client, next) {
        if (c->handle == handle) {
            client = c;
            break;
        }
    }

    ASSERT(client != NULL);
    struct message m;

    m.type = TCPIP_READ_MSG;
    m.tcpip_read.handle = client->handle;
    m.tcpip_read.len = 4096;
    ASSERT_OK(ipc_call(tcpip_server, &m));
    uint8_t *buf = (uint8_t *) m.tcpip_read_reply.data;
    size_t len = m.tcpip_read_reply.data_len;
    if (buf) {
        process(client, buf, len);
        free(buf);
    }
}

void main(void) {
    TRACE("starting...");
    tcpip_server = ipc_lookup("tcpip");
    list_init(&clients);

    struct message m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.port = 80;
    m.tcpip_listen.backlog = 16;
    ASSERT_OK(ipc_call(tcpip_server, &m));
    INFO("ready");
    while (1) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG: {
                if (m.notifications.data & NOTIFY_ASYNC) {
                    ASSERT_OK(async_recv(tcpip_server, &m));
                    switch (m.type) {
                        case TCPIP_RECEIVED_MSG:
                            tcp_read(m.tcpip_received.handle);
                            break;
                        case TCPIP_NEW_CLIENT_MSG: {
                            m.type = TCPIP_ACCEPT_MSG;
                            m.tcpip_accept.handle = m.tcpip_new_client.handle;
                            ASSERT_OK(ipc_call(tcpip_server, &m));
                            handle_t new_handle =
                                m.tcpip_accept_reply.new_handle;

                            struct client *client = malloc(sizeof(*client));
                            client->handle = new_handle;
                            client->request = NULL;
                            client->request_len = 0;
                            client->done = false;
                            list_push_back(&clients, &client->next);
                            tcp_read(client->handle);
                            break;
                        }
                        case TCPIP_CLOSED_MSG: {
                            LIST_FOR_EACH (c, &clients, struct client, next) {
                                if (c->handle == m.tcpip_closed.handle) {
                                    list_remove(&c->next);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            };
            default:
                discard_unknown_message(&m);
        }
    }
}
