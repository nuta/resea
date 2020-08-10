# Service Discovery
The vm server implements service discovery, it allows looking for services by
their names.

## Registering a Service
```c
#include <resea/ipc.h>

error_t ipc_serve(const char *name);
```

## Looking for a Service
```c
#include <resea/ipc.h>

task_t ipc_lookup(const char *name);
```

This function blocks until the server with the given name has been registered,
and then returns the server's task ID.
