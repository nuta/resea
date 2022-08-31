ENTRY(start)

SECTIONS {
    . = 0x10000;

    .text : {
        *(.text .text.*);
        . = ALIGN(4096);
    }

    .rodata : {
        *(.rodata .rodata.*);
        . = ALIGN(16);
        *(.srodata .srodata.*);
        . = ALIGN(4096);
    }

    .data : {
        *(.data .data.*);
        *(.sdata .sdata.*);
        . = ALIGN(4096);
    }

    .bss : {
        *(.bss .bss.*);
        . = ALIGN(16);
        *(.sbss .sbss.*);

        . = ALIGN(4096);
        . = . + 256 * 1024;
        __stack = .;
    }
}
