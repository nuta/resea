# Memory Allocation (malloc)
We provide some dynamic memory alllocation APIs.

```c
#include <resea/malloc.h>

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
```

See [a man page](https://linux.die.net/man/3/malloc) in UNIX for details.
