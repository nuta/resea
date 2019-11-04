OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

SECTIONS {
    . = 0x0000000001000000;

    .text : ALIGN(8) {
        __text = .;
        *(.text);
        *(.text.*);
        *(.got);
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

    . = 0x030000000;
    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    }

    __heap = .;
    . = 0x04000000;
    __heap_end = .;
}
