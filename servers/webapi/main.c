#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/session.h>
#include <std/syscall.h>
#include <string.h>
#include <stubs/tcpip.h>
#include <vprintf.h>
#include "webapi.h"

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

    error_t err = tcpip_write(client->handle, buf, strlen(buf));
    ASSERT_OK(err);
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

malformed:
    tcpip_close(client->handle);
}

void main(void) {
    INFO("starting...");
    tcpip_init();
    session_init();

    handle_t sock = tcpip_listen(80, 16);
    ASSERT_OK(sock);

    size_t m_len = sizeof(struct message) + TCP_DATA_LEN_MAX;
    struct message *m = malloc(m_len);
    size_t buf_len = 1024;
    uint8_t *buf = malloc(buf_len);

    INFO("ready");
    while (1) {
        error_t err = ipc_recv(IPC_ANY, m, m_len);
        ASSERT_OK(err);

        switch (m->type) {
            case TCP_NEW_CLIENT_MSG: {
                DBG("new client! %d", m->tcp_new_client.handle);
                handle_t new_handle = tcpip_accept(m->tcp_new_client.handle);
                ASSERT_OK(new_handle);

                struct client *client = malloc(sizeof(*client));
                client->handle = new_handle;
                client->request = NULL;
                client->request_len = 0;
                client->done = false;
                DBG("new_handle: %d", new_handle);
                struct session *sess =
                    session_alloc_at(tcpip_server(), new_handle);
                ASSERT(sess);
                sess->data = client;
                break;
            }
            case TCP_CLOSED_MSG: {
                handle_t handle = m->tcp_closed.handle;
                struct session *sess = session_get(tcpip_server(), handle);
                struct client *client = (struct client *) sess->data;
                ASSERT(client);
                free(client->request);
                free(client);
                session_delete(tcpip_server(), handle);
                break;
            }
            case TCP_RECEIVED_MSG: {
                DBG("new data");
                struct session *sess =
                    session_get(tcpip_server(), m->tcp_received.handle);
                struct client *client = (struct client *) sess->data;
                ASSERT(client);

                tcpip_read(client->handle, buf, buf_len);
                process(client, buf, buf_len);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m->type);
        }
    }
}
