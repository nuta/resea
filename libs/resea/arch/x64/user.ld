OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    /* XXX: For simplicity of the implementation, kernel's ELF loader assumes
            the address and the length of program headers are aligned to 4096
            (PAGE_SIZE). */

    . = 0x01000000;

    .text : ALIGN(0x1000) {
        __text = .;
        *(.text);
        *(.text.*);
        __text_end = .;
        . = ALIGN(4096); // See the XXX comment above.
    } :text

    .rodata : ALIGN(0x1000) {
        *(.rodata);
        *(.rodata.*);
        . = ALIGN(4096); // See the XXX comment above.
    } :text

    . = 0x03000000;
    .data : ALIGN(0x1000) {
        *(.data);
        *(.data.*);
        . = ALIGN(4096); // See the XXX comment above.
    } :data

    __zeroed_pages = .;

    .bss : ALIGN(0x1000) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
        . = ALIGN(4096); // See the XXX comment above.
    } :bss

    . = ALIGN(16);
    __stack = .;
    . += 0x4000;
    __stack_end = .;

    . = ALIGN(16);
    __heap = .;
    . = 0x05000000;
    __heap_end = .;

    __free_vaddr = .;
    . = 0x0a000000;
    __free_vaddr_end = .;
}
