#!/usr/bin/env python3
# Modifies em_machine in the ELF header to pretend to be an x86 elf image in
# order to make it bootable in QEMU: it denies a x86_64 multiboot elf file
# (GRUB2 boots the kernel image without this hack, btw):
# https://github.com/qemu/qemu/blob/950c4e6c94b15cd0d8b63891dddd7a8dbf458e6a/hw/i386/multiboot.c#L197
import argparse
import shutil

def main():
    parser = argparse.ArgumentParser(description="Make a x86_64 kernel image bootable on QEMU.")
    parser.add_argument("infile", help="The kernel image file.")
    parser.add_argument("outfile", help="The output file.")
    args = parser.parse_args()

    shutil.copyfile(args.infile, args.outfile)
    with open(args.outfile, 'r+b') as f:
        # Set EM_386 (0x0003) to em_machine.
        f.seek(18)
        f.write(bytes([0x03, 0x00]))

if __name__ == "__main__":
    main()
