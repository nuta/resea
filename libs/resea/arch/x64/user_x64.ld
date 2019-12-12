OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

SECTIONS {
    . = 0x01000000;

    .text : ALIGN(8) {
        __text = .;
        *(.text);
        *(.text.*);
        __text_end = .;
    }

    .rodata : ALIGN(8) {
        *(.rodata);
        *(.rodata.*);
    }

    .data : ALIGN(8) {
        *(.data);
        *(.data.*);
    }

    . = 0x03000000;
    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    }

    __heap = .;
    . = 0x07000000; __heap_end = .;
    . = 0x07000000; __writable_pages = .;
    . = 0x0b000000; __writable_pages_end = .;
    . = 0x50000000; __unmapped_pages = .;
    . = 0x70000000; __unmapped_pages_end = .;
}
