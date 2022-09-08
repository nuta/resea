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

        . = ALIGN(16);
        __unit_tests = .;
        *(.unit_tests);
        __unit_tests_end = .;
    }

    .data : ALIGN(4096) {
        *(.data .data.*);
        *(.sdata .sdata.*);
    }

    .bss : ALIGN(4096) {
        *(.bss .bss.*);
        . = ALIGN(16);
        *(.sbss .sbss.*);

        __heap = .;
        . += 0x20000;
        __heap_end = .;

        . = . + 256 * 1024;
        __stack = .;
    }
}
