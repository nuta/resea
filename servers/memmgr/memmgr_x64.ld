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

    . = 0x03500000;
    __heap = .;
    . = 0x04000000;
    __heap_end = .;

    . = 0xa0000000;
    __valloc = .;
    . = 0xb0000000;
    __valloc_end = .;
}
