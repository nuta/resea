MEMORY {
    rom : ORIGIN = 0x00011000, LENGTH = 0x30000
    ram : ORIGIN = 0x20002000, LENGTH = 0x2000
}

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
    // TODO: Unsupported in arm port for now.
    __cmdline = 0;

    .text : ALIGN(0x1000) {
        *(.bootelf_header)
        *(.text*)
    } > rom :text

    .rodata :{
        *(.rodata*)
    } > rom :text

    .data : ALIGN(0x1000) {
        *(.data*)
    } > ram :data

    .bss : ALIGN(0x1000) {
        __bss = .;
        *(.bss*);
        __bss_end = .;

        . = ALIGN(4);
        __heap = .;
        . += 4096;
        __heap_end = .;
    } > ram :bss

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
