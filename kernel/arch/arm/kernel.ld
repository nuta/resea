MEMORY {
    /* Don't forget to update libs/resea/arch/arm/user.ld */
    rom : ORIGIN = 0x00000000, LENGTH = 0x10000
    app : ORIGIN = 0x00010000, LENGTH = 0x30000
    ram : ORIGIN = 0x20000000, LENGTH = 0x2000
}

SECTIONS {
    .text :{
        *(.vector*)
        *(.text*)
    } > rom

    .rodata :{
        *(.rodata*)
    } > rom

    .bootelf :{
        *(.bootelf)
    } > app

    .stack :{
        . += 1024;
        __boot_stack_top = .;
    } > ram

    .data :{
        *(.data*)
    } > ram

    .bss :{
        __kernel_heap = .;
        . += 0x1800;
        __kernel_heap_end = .;
        *(.bss*)
    } > ram

    // We don't use __temp_page in NOMMU
    . = 0;
    __temp_page;

    /DISCARD/ :{
        *(.ARM.exidx)
    }
}
