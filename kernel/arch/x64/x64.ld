OUTPUT_FORMAT("elf64-x86-64");
ENTRY(x64_boot);

LMA_OFFSET = 0xffff800000000000;
KERNEL_BOOT_ADDR = 0x100000;
KERNEL_BASE_ADDR = 0xffff800000100000;

PHDRS {
	boot PT_LOAD FLAGS(5); /* flags = rx */
	text PT_LOAD FLAGS(5); /* flags = rx */
	data PT_LOAD FLAGS(6); /* flags = rw */
}

SECTIONS {
    . = KERNEL_BOOT_ADDR;
    .boot : AT(KERNEL_BOOT_ADDR) {
        KEEP(*(.boot));
    } :boot

    . = KERNEL_BASE_ADDR + SIZEOF(.boot);

    __kernel_image_start = .;
    .text : AT(ADDR(.text) - LMA_OFFSET) ALIGN(8) {
        *(.text);
        *(.text.*);
    } :text

    .rodata : AT(ADDR(.rodata) - LMA_OFFSET) ALIGN(8) {
        *(.rodata);
        *(.rodata.*);
    } :text

    .data : AT(ADDR(.data) - LMA_OFFSET) ALIGN(8) {
        *(.data);
        *(.data.*);
    } :data

    .bss : AT(ADDR(.bss) - LMA_OFFSET) ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    } :data
    __kernel_image_end = .;

    /DISCARD/ :{
        *(*.eh_frame);
    }
}
