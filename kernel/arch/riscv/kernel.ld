ENTRY(boot)

SECTIONS {
    . = 0x80000000;

    .text : {
        // This part should be first to make sure that the boot code is loaded
        // at 0x80000000.
        KEEP(*(.boot));

        . = ALIGN(16);
        *(.text .text.*);
    }

    .rodata : {
        . = ALIGN(16);
        *(.rodata .rodata.*);
        . = ALIGN(16);
        *(.srodata .srodata.*);
        . = ALIGN(4096);
        *(.boot_elf);
    }

    .data : {
        . = ALIGN(16);
        *(.data .data.*);
        . = ALIGN(16);
        *(.sdata .sdata.*);
    }

    .bss : {
        . = ALIGN(16);
        *(.bss .bss.*);
        . = ALIGN(16);
        *(.sbss .sbss.*);

        . = ALIGN(4096);
        . = . + 256 * 1024;
        __boot_stack = .;
    }
}
