ENTRY(boot);

LMA_OFFSET = 0xffff000000000000;

MEMORY {
    boot_ram (rwx): ORIGIN = 0x0000000000080000, LENGTH = 0x0001000
    ram (rwx):      ORIGIN = 0xffff000000081000, LENGTH = 0x2fff000
    lma_ram (rwx):  ORIGIN = 0x0000000000081000, LENGTH = 0x2fff000
}

SECTIONS {
    .boot : {
        __kernel_image = .; /* paddr_t */
        KEEP(*(.boot));
    } > boot_ram

    .text : ALIGN(8) {
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

        /* Kernel paging table. */
        . = ALIGN(4096);
        __kernel_page_table = . - LMA_OFFSET;
        . += 0x4000;

        /* Per-CPU boot stacks (paddr_t)/ */
        __boot_stack_base = . - LMA_OFFSET;
        . += 0x1000 * 16; /* PAGE_SIZE * CPU_NUM_MAX */

        . = ALIGN(4096);
        __kernel_image_end = . - LMA_OFFSET; /* paddr_t */
    } > ram AT > lma_ram

    /DISCARD/ :{
        *(*.eh_frame);
    }
}
