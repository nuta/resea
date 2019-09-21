#ifndef __DRIVER_IO_H__
#define __DRIVER_IO_H__

#include <types.h>

#ifdef __x86_64__
#include <driver/io_x64.h>
#else
#    error "unsupported CPU architecture"
#endif

enum io_space_type {
    IO_SPACE_IOPORT,
    IO_SPACE_MEMORY,
};

struct io_handle {
    enum io_space_type type;
    uintptr_t addr;
    size_t size;
};

typedef struct io_handle io_handle_t;

error_t io_open(io_handle_t *handle, cid_t io_server, enum io_space_type type,
                uintptr_t addr, size_t size);
void io_close(io_handle_t *handle);
uint8_t io_read8(io_handle_t *handle, size_t offset);
uint16_t io_read16(io_handle_t *handle, size_t offset);
uint32_t io_read32(io_handle_t *handle, size_t offset);
void io_write8(io_handle_t *handle, size_t offset, uint8_t value);
void io_write16(io_handle_t *handle, size_t offset, uint16_t value);
void io_write32(io_handle_t *handle, size_t offset, uint32_t value);

error_t io_listen_interrupt(cid_t io_server, cid_t receive_at, int irq,
                            cid_t *new_ch);

#endif
