OUTPUT_FORMAT("elf64-x86-64");
ENTRY(inifs_jump_code);

PHDRS {
	startup PT_LOAD FLAGS(5); /* flags = rx */
	text PT_LOAD FLAGS(5); /* flags = rx */
	data PT_LOAD FLAGS(6); /* flags = rw */
}

SECTIONS {

    . = 0x100000;

    .startup : {
        KEEP(*(.startup));
    } :startup

    .text : ALIGN(8) {
        *(.text);
    } :text

    .rodata : ALIGN(8) {
        *(.rodata);
        *(.rodata.*);
    } :text

    .data : ALIGN(8) {
        *(.data);
        *(.data.*);
    } :data

    .bss : ALIGN(8) {
        __bss = .;
        *(.bss);
        *(.bss.*);
        __bss_end = .;
    } :data

    .got (NOLOAD) : {
      KEEP(*(.got .got.*));
    }

    /DISCARD/ :{
        *(*.eh_frame);
    }
}