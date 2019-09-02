#ifndef __ELF_H__
#define __ELF_H__

#include <types.h>

#define EI_NIDENT 16

struct elf64_ehdr {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} PACKED;

struct elf_file {
    uint64_t entry;
    int num_phdrs;
    struct elf64_phdr *phdrs;
};

error_t elf_parse(struct elf_file *elf, const void *image, size_t image_len);

#endif
