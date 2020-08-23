#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/io.h>

io_t io_alloc_port(unsigned long base, size_t len, unsigned flags) {
    struct io *io = malloc(sizeof(*io));
    io->space = IO_SPACE_IO;
    io->port.base = base;
    return io;
}

io_t io_alloc_memory(size_t len, unsigned flags) {
    return io_alloc_memory_fixed(0, len, flags);
}

io_t io_alloc_memory_fixed(paddr_t paddr, size_t len, unsigned flags) {
    struct message m;
    m.type = ALLOC_PAGES_MSG;
    m.alloc_pages.paddr = paddr;
    m.alloc_pages.num_pages = ALIGN_UP(len, PAGE_SIZE)/ PAGE_SIZE;
    error_t err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    ASSERT(m.type == ALLOC_PAGES_REPLY_MSG);

    struct io *io = malloc(sizeof(*io));
    io->space = IO_SPACE_MEMORY;
    io->memory.paddr = m.alloc_pages_reply.paddr;
    io->memory.vaddr = m.alloc_pages_reply.vaddr;
    return io;
}

void io_write8(io_t io, offset_t offset, uint8_t value) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            *((volatile uint8_t *) (io->memory.vaddr + offset)) = value;
            break;
    }
}

void io_write16(io_t io, offset_t offset, uint16_t value) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("outw %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            *((volatile uint16_t *) (io->memory.vaddr + offset)) = value;
            break;
    }
}

void io_write32(io_t io, offset_t offset, uint32_t value) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("outl %0, %1" :: "a"(value), "Nd"(port));
            break;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            *((volatile uint32_t *) (io->memory.vaddr + offset)) = value;
            break;
    }
}

uint8_t io_read8(io_t io, offset_t offset) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint8_t value;
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint8_t *) (io->memory.vaddr + offset));
    }
}

uint16_t io_read16(io_t io, offset_t offset) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint16_t value;
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint16_t *) (io->memory.vaddr + offset));
    }
}

uint32_t io_read32(io_t io, offset_t offset) {
    DEBUG_ASSERT(io != NULL);
    switch (io->space) {
        case IO_SPACE_IO: {
#ifdef CONFIG_ARCH_X64
            uint32_t value;
            uint16_t port = io->port.base + offset;
            __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
            return value;
#else
            PANIC("port-mapped I/O is not supported");
#endif
        }
        case IO_SPACE_MEMORY:
            return *((volatile uint32_t *) (io->memory.vaddr + offset));
    }
}

/// Performs arch-specific flushing after writing into the DMA area.
void io_flush_read(io_t io) {
    // Add arch-specific task with #ifdef if you need.
    switch (io->space) {
        case IO_SPACE_MEMORY:
            // Prevent reordering the memory access.
            __sync_synchronize();
            break;
        case IO_SPACE_IO:
            // Nothing to do.
            break;
    }
}

/// Performs arch-specific flushing before reading from the DMA area.
void io_flush_write(io_t io) {
    // Add arch-specific task with #ifdef if you need.
switch (io->space) {
    case IO_SPACE_MEMORY:
        // Prevent reordering the memory access.
        __sync_synchronize();
        break;
    case IO_SPACE_IO:
        // Nothing to do.
        break;
}
}

