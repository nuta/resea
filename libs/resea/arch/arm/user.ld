MEMORY {
    /* Copied from libs/resea/arch/arm/user.ld */
    rom : ORIGIN = 0x00011000, LENGTH = 0x30000
    ram : ORIGIN = 0x20002000, LENGTH = 0x2000
}

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS {
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
        . = ALIGN(4096); // See the XXX comment above.
        __heap_end = .;
    } > ram :bss

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
