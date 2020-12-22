#ifndef __IOPORT_H__
#define __IOPORT_H__

#include <types.h>

struct guest;
uint32_t handle_io_read(struct guest *guest, uint64_t port, size_t size);
void handle_io_write(struct guest *guest, uint64_t port, size_t size,
                     uint32_t value);
#endif
