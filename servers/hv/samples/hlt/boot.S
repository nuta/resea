.section ".boot", "ax"
.intel_syntax noprefix

#define MULTIBOOT_MAGIC    0x1badb002
#define MULTIBOOT_FLAGS    0x00000002 // memory map
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.code32

// The multiboot header.
.align 8
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

.code32
.global boot
boot:
    // Send "Hi\n" to the serial port.
    mov dx, 0x3f8
    mov bl, 'i'
    mov cl, '\n'

    mov al, 'H'
    out dx, al

    mov al, bl
    out dx, al

    mov al, cl
    out dx, al

1:
    hlt
    jmp 1b
