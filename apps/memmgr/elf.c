#include "elf.h"

#include <resea.h>
#include <resea/string.h>

/// Parses a ELF image.
/// TODO: support ELF32
error_t elf_parse(struct elf_file *elf, const void *image, size_t image_len) {
    if (image_len < sizeof(struct elf64_ehdr)) {
        WARN("too short ELF image");
        return ERR_TOO_SHORT;
    }

    // Verify the magic.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) image;
    if (memcmp(&ehdr->e_ident,
            "\x7f"
            "ELF",
            4) != 0) {
        WARN("invalid ELF magic");
        return ERR_INVALID_DATA;
    }

    elf->entry = ehdr->e_entry;
    elf->num_phdrs = ehdr->e_phnum;
    elf->phdrs = (struct elf64_phdr *) ((uint8_t *) image + ehdr->e_ehsize);
    return OK;
}
