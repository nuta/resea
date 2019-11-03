Server Writer's Guide
======================

An user program is called *server* if it provides services to other processes.
Note that there're no differences between apps and servers: it's just
[a terminology used in microkernels](https://en.wikipedia.org/wiki/Microkernel#Servers).

### Getting started
1. Create essential files in `servers` directory:
```
servers
└── <name>
    ├── Kconfig
    ├── server.mk
    └── main.c
```

2. Edit `server.mk` and `Kconfig`:
```makefile
name := <name>   # The server name.
objs := main.o   # Source files.
libs := libresea # Dependent libraries.
```

```
menuconfig <NAME>_SERVER
    bool "The <name> server"
    default y
```

3. Write a program in `main.c`:
```c
#include <resea.h>

int main(void) {
    printf("Hello World!\n");
    return 0;
}
```

4. Enable the server in the build config.
```
$ make menuconfig
```

5. Build and run!
```
$ make -j8 run
```

### Interface Definition Language (IDL)
Parsing and constructing IPC messages are really annoying and painful work. To improve
the productivity, Resea provides the stub generator ([tools/genstub.py](https://github.com/seiyanuta/resea/blob/master/tools/genstub.py)). It generates message definitions and stub functions
from the IDL file ([interface.idl](https://github.com/seiyanuta/resea/blob/master/interfaces.idl)).


#### Using IDL stubs
To load IDL stubs, include `idl_stubs.h`:
```c
#include <idl_stubs.h>
```

### Programming model
We recommend write a server in single-threaded, event-driven programming
model. It's straightforward and makes really easy to debug.

TODO: Add a sample code.
