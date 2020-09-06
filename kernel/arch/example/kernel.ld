OUTPUT_FORMAT("elf64-x86-64");
ENTRY(boot);

LMA_OFFSET = 0xffff800000000000;

MEMORY {
    ram (rwx):      ORIGIN = 0xffff800000100000, LENGTH = 0x2000000
    lma_ram (rwx):  ORIGIN = 0x0000000000100000, LENGTH = 0x2000000
}

SECTIONS {
    .text : ALIGN(8) {
        __kernel_image = .; /* paddr_t */
        *(.text);
        *(.text.*);
    } > ram AT > lma_ram

    .rodata : ALIGN(8) {
        *(.rodata);
        *(.rodata.*);
    } > ram AT > lma_ram

    .data : ALIGN(8) {
        *(.data);
        *(.data.*);
    } > ram AT > lma_ram

    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
        __kernel_image_end = . - LMA_OFFSET; /* paddr_t */
    } > ram AT > lma_ram

    /DISCARD/ :{
        *(*.eh_frame);
    }
}
