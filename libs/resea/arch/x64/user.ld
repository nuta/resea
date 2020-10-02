OUTPUT_FORMAT("elf64-x86-64");
ENTRY(start);

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    . = 0x01000000;

    .text : ALIGN(0x1000) {
        __text = .;
        *(.bootelf_header);
        *(.text);
        *(.text.*);
        __text_end = .;
    } :text

    .rodata : ALIGN(0x1000) {
        *(.rodata);
        *(.rodata.*);
    } :text

    __cmdline = 0x02fff000;

    . = 0x03000000;
    .data : ALIGN(0x1000) {
        *(.data);
        *(.data.*);

        . = ALIGN(16);
        __stack = .;
        . += 0x4000;
        __stack_end = .;
    } :data

    . = 0x04000000;
    __zeroed_pages = .;
    .bss : ALIGN(0x1000) {
        __heap = .;
        . += 0x100000;
        __heap_end = .;

        . = ALIGN(0x1000);
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    } :bss

    . = 0x05000000;
    __zeroed_pages_end = .;
    __straight_mapping = .;

    __free_vaddr = .;
    . = 0x0a000000;
    __free_vaddr_end = .;
}
