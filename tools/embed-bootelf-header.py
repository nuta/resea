#!/usr/bin/env python3
import argparse
import struct
from elftools.elf.elffile import ELFFile

PAGE_SIZE = 0x1000 # FIXME: This could be other values.
BOOTELF_PRE_MAGIC = b"00BOOT\xe1\xff"
BOOTELF_MAGIC = b"11BOOT\xe1\xff"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    parser.add_argument("elf_file")
    args = parser.parse_args()

    elf = ELFFile(open(args.elf_file, "r+b"))
    assert elf.num_segments() < 6

    # struct bootelf_header {
    # #define BOOTELF_MAGIC "\xaa\xbbBOOT\xe1\xff"
    #     uint8_t magic[8];
    #     uint8_t name[16];
    #     uint64_t entry;
    #     uint8_t num_mappings;
    #     uint8_t padding[7];
    #     struct bootelf_mapping mappings[];
    # } __packed;
    header = struct.pack("8s16sQB7x", BOOTELF_MAGIC, args.name.encode("ascii"),
                         elf.header.e_entry, elf.num_segments())

    for segment in elf.iter_segments():
        assert segment.header.p_vaddr > 0
        assert segment.header.p_type == 'PT_LOAD'
        assert segment.header.p_offset % PAGE_SIZE == 0
        assert segment.header.p_memsz == segment.header.p_filesz or \
            segment.header.p_filesz == 0

        # struct bootelf_mapping {
        #     uint64_t vaddr;
        #     uint32_t offset;
        #     uint16_t num_pages;
        #     uint8_t type;   // 'R': readonly, 'W': writable, 'X': executable.
        #     uint8_t zeroed; // If it's non-zero value, the pages are filled with zeros.
        # } __packed;
        vaddr = segment.header.p_vaddr
        offset = segment.header.p_offset
        num_pages = (segment.header.p_memsz + PAGE_SIZE - 1) // PAGE_SIZE
        zeroed = segment.header.p_filesz == 0
        header += struct.pack("QIHcb", vaddr, offset, num_pages,
                              b'W', # TODO: Support so-called W^X
                              1 if zeroed else 0)

    with open(args.elf_file, "r+b") as f:
        for offset in [0x1000, 0x10000]:
            f.seek(offset)
            if f.read(8) == BOOTELF_PRE_MAGIC:
                break

        f.seek(offset)
        assert f.read(8) == BOOTELF_PRE_MAGIC
        f.seek(offset)
        f.write(header)

if __name__ == "__main__":
    main()
