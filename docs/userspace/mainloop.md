# Mainloop
First, let's write the mainloop! You can use the template in `servers/example`.

```c
// main.c
#include <resea/ipc.h>
#include <resea/printf.h>
#include <string.h>

void main(void) {
    TRACE("starting...");

    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case BENCHMARK_NOP_MSG:
                m.type = BENCHMARK_NOP_REPLY_MSG;
                m.benchmark_nop_reply.value = 123456789;
                ipc_reply(m.src, &m);
                break;
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
```

From this short snippet, we can learn how servers (and applications) are written
in Resea:

- In userspace programming, we'll use *Resea Standard Library* (`libs/resea`).
  In C, they're available in `<resea/*.h>` header files.
  - `<resea/ipc.h>`: Message passing APIs such as `ipc_recv`.
  - `<resea/printf.h>`: Print/assertion macros such as `INFO`, `TRACE`, and `ASSERT_OK`.
- The message passing APIs (`ipc_recv` and `ipc_reply`) are blocking and the message is
  fixed-sized (32-256 bytes, depends on arch).
- If you re-use the message buffer `m` to reply a message to the client,
  clear it with `bzero` to prevent information leak.
- We recommend to write a program in single-threaded event-driven programming,
  that is, receive a message, handle it, reply, and then wait for a new one, ...
  - It significantly reduces the cost of debugging complicated software like
    multi-server operating system. See [John Ousterhout. Why Threads Are A Bad Idea (for most purposes)](https://web.stanford.edu/~ouster/cgi-bin/papers/threads.pdf).
