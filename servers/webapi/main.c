#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/map.h>
#include <std/syscall.h>
#include <std/lookup.h>
#include <cstring.h>
#include <vprintf.h>
#include "webapi.h"

static map_t clients;
static task_t network_server;

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

    struct ipc_msg_t m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.handle = client->handle;
    m.tcpip_write.data = buf;
    m.tcpip_write.len = strlen(buf);
    ASSERT_OK(ipc_call(network_server, &m));
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

    DBG("client->request = %s", client->request);

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

    struct ipc_msg_t m;
malformed:
    m.type = TCPIP_CLOSE_MSG;
    m.tcpip_close.handle = client->handle;
    ipc_call(network_server, &m);
}

void main(void) {
    INFO("starting...");
    network_server = ipc_lookup("network");
    clients = map_new();

    struct ipc_msg_t m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.port = 80;
    m.tcpip_listen.backlog = 16;
    ASSERT_OK(ipc_call(network_server, &m));
    INFO("ready");
    while (1) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG: {
                if (m.notifications.data & NOTIFY_NEW_DATA) {
                    m.type = TCPIP_PULL_MSG;
                    ASSERT_OK(ipc_call(network_server, &m));
                    switch (m.type) {
                        case TCPIP_RECEIVED_MSG: {
                            DBG("new data");
                            struct client *c =
                                map_get_handle(clients, &m.tcpip_received.handle);
                            ASSERT(c);

                            m.type = TCPIP_READ_MSG;
                            m.tcpip_read.handle = c->handle;
                            m.tcpip_read.len = 4096;
                            ASSERT_OK(ipc_call(network_server, &m));
                            uint8_t *buf = m.tcpip_read_reply.data;
                            size_t len = m.tcpip_read_reply.len;
                            if (buf) {
                                process(c, buf, len);
                                free(buf);
                            }
                            break;
                        }
                        case TCPIP_NEW_CLIENT_MSG: {
                            m.type = TCPIP_ACCEPT_MSG;
                            m.tcpip_accept.handle = m.tcpip_new_client.handle;
                            ASSERT_OK(ipc_call(network_server, &m));
                            handle_t new_handle = m.tcpip_accept_reply.new_handle;

                            struct client *client = malloc(sizeof(*client));
                            client->handle = new_handle;
                            client->request = NULL;
                            client->request_len = 0;
                            client->done = false;
                            map_set_handle(clients, &new_handle, client);
                            break;
                        }
                        case TCPIP_CLOSED_MSG: {
                            map_remove_handle(clients, &m.tcpip_closed.handle);
                            break;
                        }
                    }
                }
                break;
            };
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
