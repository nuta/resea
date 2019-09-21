#include <resea.h>
#include <resea_idl.h>
#include <driver/io.h>

error_t io_open(io_handle_t *handle, cid_t io_server, enum io_space_type type,
                uintptr_t addr, size_t size) {
    switch (type) {
    case IO_SPACE_IOPORT:
        TRY(call_io_allow_iomapped_io(io_server));
        break;
    case IO_SPACE_MEMORY:
        // TODO: Support memory-mapped IO (IO_SPACE_MEMORY).
        UNIMPLEMENTED();
    }

    handle->type = type;
    handle->addr = addr;
    handle->size = size;
    return OK;
}

void io_close(io_handle_t *handle) {
    switch (handle->type) {
    case IO_SPACE_IOPORT:
        // Nothing to do.
        break;
    case IO_SPACE_MEMORY:
        UNIMPLEMENTED();
    }
}

uint8_t io_read8(io_handle_t *handle, size_t offset) {
    assert(handle->type == IO_SPACE_IOPORT);
    return arch_ioport_read8(handle->addr + offset);
}

uint16_t io_read16(io_handle_t *handle, size_t offset) {
    assert(handle->type == IO_SPACE_IOPORT);
    return arch_ioport_read16(handle->addr + offset);
}

uint32_t io_read32(io_handle_t *handle, size_t offset) {
    assert(handle->type == IO_SPACE_IOPORT);
    return arch_ioport_read32(handle->addr + offset);
}

void io_write8(io_handle_t *handle, size_t offset, uint8_t value) {
    assert(handle->type == IO_SPACE_IOPORT);
    arch_ioport_write8(handle->addr + offset, value);
}

void io_write16(io_handle_t *handle, size_t offset, uint16_t value) {
    assert(handle->type == IO_SPACE_IOPORT);
    arch_ioport_write16(handle->addr + offset, value);
}

void io_write32(io_handle_t *handle, size_t offset, uint32_t value) {
    assert(handle->type == IO_SPACE_IOPORT);
    arch_ioport_write32(handle->addr + offset, value);

}

error_t io_listen_interrupt(cid_t io_server, cid_t receive_at, int irq,
                            cid_t *new_ch) {
    cid_t ch;
    TRY(open(&ch));
    TRY(transfer(ch, receive_at));
    TRY(call_io_listen_irq(io_server, ch, irq));
    *new_ch = ch;
    return OK;
}
