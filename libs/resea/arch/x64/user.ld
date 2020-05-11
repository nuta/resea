OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

SECTIONS {
    . = 0x01000000;

    .initfs : ALIGN(8) {
        *(.initfs);
    }

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
    __zeroed_pages = .;

    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    }

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
