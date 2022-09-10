ENTRY(boot)

SECTIONS {
    . = 0x80000000;

    .text : {
        // This part should be first to make sure that the boot code is loaded
        // at 0x80000000.
        __text = .;
        KEEP(*(.boot));

        . = ALIGN(16);
        *(.text .text.*);

        . = ALIGN(4096);
        __text_end = .;
    }

    .rodata : {
        *(.rodata .rodata.*);
        . = ALIGN(16);
        *(.srodata .srodata.*);

        . = ALIGN(4096);
        __boot_elf = .;
        *(.boot_elf);
        . = ALIGN(4096);
    }

    .data : {
        . = ALIGN(16);
        *(.data .data.*);
        . = ALIGN(16);
        *(.sdata .sdata.*);
    }

    .bss : {
        __bss = .;
        . = ALIGN(16);
        *(.bss .bss.*);
        . = ALIGN(16);
        *(.sbss .sbss.*);
        __bss_end = .;

        . = ALIGN(4096);
        . = . + 256 * 1024;
        __boot_stack = .;
    }

    . = ALIGN(4096);
    __data_end = .;
    __image_end = .;
}
