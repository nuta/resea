ENTRY(start)

SECTIONS {
    . = 0x10000;

    .text : ALIGN(4096) {
        *(.text .text.*);
    }

    .rodata : ALIGN(4096) {
        *(.rodata .rodata.*);
        . = ALIGN(16);
        *(.srodata .srodata.*);
    }

    .data : ALIGN(4096) {
        *(.data .data.*);
        *(.sdata .sdata.*);
    }

    .bss : ALIGN(4096) {
        *(.bss .bss.*);
        . = ALIGN(16);
        *(.sbss .sbss.*);

        . = . + 256 * 1024;
        __stack = .;
    }
}
