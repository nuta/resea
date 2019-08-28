Hacking
=======

This document describes how to add new features to Resea.

The Resea standard library (`libresea`)
---------------------------------------
`libresea` provides fundamental features for apps and libraries. It does not
aims to be compatibile with C standard library as Resea kernel itself does not
rich features that UNIX provides (the concept of *file*, for example).

### Print functions
```c
// Prefer these printf-like macros to printf.
#define UNIMPLEMENTED()  // Exits the program.
#define BUG(fmt, ...)    // Exits the program with a stack trace.
#define ERROR(fmt, ...)  // Exits the program with a stack trace.
#define OOPS(fmt, ...)   // Prints a message with stack trace.
#define WARN(fmt, ...)
#define INFO(fmt, ...)
#define DEBUG(fmt, ...)  // Only available in debug build.
#define TRACE(fmt, ...)  // Only available in debug build.

void putchar(char ch);
void puts(const char *s);
void printf(const char *fmt, ...);
```

### Math
```c
#define min(a, b) // Supports arbitrary integer types.
#define max(a, b) // Supports arbitrary integer types.
```

### Exit the program
```c
void exit(int status);
// Aborts the program if `err` is not OK.
#define TRY_OR_PANIC(err)
```

## IPC (system calls)
```c
// Include `resea/ipc.h` to enable:
#include <resea/ipc.h>

struct message;

// Creates a new channel.
error_t open(cid_t *ch);
// Transfer messages sent to `src` to `dst`.
error_t transfer(cid_t src, cid_t dst);
// Receive a message.
error_t ipc_recv(cid_t ch, struct message *r);
// Send and receive a message.
error_t ipc_call(cid_t ch, struct message *m, struct message *r);
// Send and receive a message. The difference from the `ipc_call` is that in
// send phase, `ipc_replyrecv` won't block (returns an error) if the receiver
// does not exist.
error_t ipc_replyrecv(cid_t ch, struct message *m, struct message *r);
```

### Interface Definition Language (IDL)
Parsing and constructing IPC messages are really annoying and painful work. To improve
the productivity, Resea provides the stub generator ([tools/genstub.py](https://github.com/seiyanuta/resea/blob/master/tools/genstub.py)).
It generates message definitions and stub functions from the IDL file ([interface.idl](https://github.com/seiyanuta/resea/blob/master/interfaces.idl)).

----

Writing an app
--------------

### Getting started
1. Create essential files in `apps` directory:
```
apps
└── <name>
    ├── app.mk
    └── main.c
```

2. Edit `app.mk`:
```makefile
name := <name>   # The app name. 
objs := main.o   # Source files.
libs := libresea # Dependent libraries.
```

3. Write a program in `main.c`:
```c
#include <resea.h>

int main(void) {
    printf("Hello World!\n");
    return 0;
}
```

4. Append `<name>` to `APPS` in `.config` to build the app:
```makefile
APPS := foo bar <name>
```

5. Build and run!
```
$ make -j8 run
```

### Using IDL stubs
To load IDL stubs, include `resea_idl.h`:
```c
#include <resea_idl.h>
```

In the IDL stubs the following macros and functions are definied:
- `<INTERFACE_NAME>_INTERFACE`
  - The interface ID.
- `<MESSAGE_NAME>_MSG`
  - The label for the message.
- `<MESSAGE_NAME>_HEADER`
  - The header for the message.
- `<MESSAGE_NAME>_REPLY_MSG`
  - The label for the reply message (if exists).
- `<MESSAGE_NAME>_REPLY_HEADER`
  -  The header for the reply message (if exists).
- `struct <message_name>_msg`
  - The message structure of the message.
- `struct <message_name>_reply_msg`
  - The message structure of the reply message (if exists).
- `error_t <message_name>(cid_t ch, args..., rets...)`
  - The call stub function. *rets* are pointers to retrieve the return values.

----

Writing a server
----------------
An app is called *server* if it provides services to other processes. Note that
there're no differences between app and server in the Resea world: it's just
[a terminology used in microkernels](https://en.wikipedia.org/wiki/Microkernel#Servers).

### Getting started
Follow steps in [Writing an app](#writing-an-app).

### Programming model
We recommend write a server in single-threaded, event-driven programming
model. It's straightforward and makes really easy to debug.

### Example
```c
#include <resea.h>
#include <resea_idl.h>

// The message handler. It responsibles to do something useful and fill the
// reply message (`r`).
static error_t handle_printchar(struct printchar_msg *m,
                                UNUSED struct printchar_reply_msg *r) {
    /* TODO: Print m->ch! */
    return OK;
}


void main(void) {
    // Create a channel to receive and reply messages from clients.
    cid_t server_ch;
    TRY_OR_PANIC(open(&server_ch));

    // Register the server channel to allow other processes to access this
    // server. Note that registering the server is NOT YET IMPLEMENTED!
    //
    // At some time, you'll be able to register your device driver, file system
    // driver, etc.
    //
    // register_server(server_ch);

    // Receive the first message.
    struct message m;    
    TRY_OR_PANIC(ipc_recv(server_ch, &m));
    while (1) {
        struct message r;
        error_t err;
        switch (MSG_LABEL(m.header)) {
        case PRINTCHAR_MSG:
            // `dispatch_*` calls the handler specified by the first argument
            // after verifying the message.
            err = dispatch_printchar(handle_printchar, &m, &r);
            break;
        default:
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            // Don't reply message if the message handler returned
            // ERR_DONT_REPLY. This will block the sender thread if the
            // message is not one-way. This is a particulary useful property
            // when we need to call another servers to reply (for example,
            // a file system server would need to call a device driver
            // to read the disk before replying a "read a file" request).
            TRY_OR_PANIC(ipc_recv(m.from, &m));
        } else {
            TRY_OR_PANIC(ipc_replyrecv(m.from, &r, &m));
        }
    }
}
```

----

Writing a library
-----------------

### Getting started
1. Create essential files in `libs` directory:
```
libs
└── <name>
    ├── lib.mk
    └── foo.c
```

2. Edit `lib.mk`:
```makefile
name := <name>   # The libs name. 
objs := main.o   # Source files.
```

3. That's it! Use it in an app as described in [Writing an app](#writing-an-app).
