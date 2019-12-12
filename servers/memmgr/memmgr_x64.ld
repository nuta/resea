OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start_memmgr);

SECTIONS {
    . = 0x0000000001000000;

    .initfs : ALIGN(8) {
        KEEP(*(.initfs));
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

    . = 0x03400000;
    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    }

    . = 0x03500000; __heap = .;
    . = 0x03a00000; __heap_end = .;
    . = 0x03a00000; __writable_pages = .;
    . = 0x04000000; __writable_pages_end = .;
    . = 0x50000000; __unmapped_pages = .;
    . = 0x70000000; __unmapped_pages_end = .;
}
