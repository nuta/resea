# Timer
Kernel provides a primitive timer feature. You can set the timer for the current task through `timer_set` API.


## Header File
```c
#include <resea/timer.h>
```

## API
```c
error_t timer_set(msec_t timeout);
```

Where `timeout` is the timeout value in milliseconds. After `timeout` milliseconds has passed, kernel notifies the task by a `NOTIFY_TIMER` notification.

Note that this is an oneshot timer (like JavaScript's `setTimeout`): you need to call `timer_set` again if you need interval timer.

Also, currently **you can't set multiple timers**.

## Example
```c
#include <config.h>
#include <resea/printf.h>
#include <resea/ipc.h>
#include <resea/timer.h>

void main(void) {
    INFO("starting a timer!");

    timer_set(1000 /* 1000ms = 1 second */);
    unsigned i = 1;
    while (true) {
        struct message m;

        // Wait until the kernel notifies us...
        ipc_recv(IPC_ANY, &m);

        if (m.type == NOTIFICATIONS_MSG) {
            if (m.notifications.data & NOTIFY_TIMER) {
                // Received a timer notification. Print a message and reset the
                // timer.
                TRACE("task's uptime: %d seconds", i++);
                timer_set(1000 /* 1000ms = 1 second */);
            }
        }
    }
}
```
