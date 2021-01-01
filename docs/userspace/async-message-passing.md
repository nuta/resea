# Asynchronous IPC
Despite the synchronous IPC works well in most cases, asynchronous message
passing is sometimes convinient.

The Resea Standard Library provides an asynchronous message pasing on top of the
sychronous message passing and notifications. See examples below for more details.

```c
#include <resea/async.h>

error_t async_send(task_t dst, struct message *m);
error_t async_recv(task_t src, struct message *m);
error_t async_reply(task_t dst);
```

In a nutshell, async library manages message queues. An async message is enqueued
and the destination task is notified that there's a pending async message.
The message will be delivered when the clients sends a pull request (`ASYNC_MSG`).

## Sending a Asynchronous Message
Enqueue a message by `async_send` and handle message pull requests (`ASYNC_MSG`)
by `async_reply`:

```c
// my_server.c

void somewhere(void) {
    // `async_send` enqueues the message and notifies the destination task with
    // the notification `NOTIFY_ASYNC`.
    m.type = BENCHMARK_NOP_MSG;
    async_send(dst, &m);
}

void main(void) {
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case ASYNC_MSG:
                // Handle a request from the client's async_recv().
                async_reply(m.src);
                break;
        }
    }
}
```

## Receiving a Asynchronous Message
Wait for `NOTIFY_ASYNC` notification and the use `ipc_recv` to receive the pending
async message:

```c
// my_client.c

void main(void) {
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_ASYNC) {
                    // Pull a pending asynchronous message from the server.
                    // As you can see, you have to know which server would
                    // send an async message in advance: you cannot determine
                    // which task has notified NOTIFY_ASYNC!
                    async_recv(my_server, &m);
                    switch (m.type) {
                        case BENCHMARK_NOP_MSG:
                            INFO("received a async message!");
                    }
                }
        }
    }
}
```
