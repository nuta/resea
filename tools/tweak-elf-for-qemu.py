#!/usr/bin/env python3
#
#  This script changes em_machine in the ELF header to pretend to
#  be an x86 elf image in order to make it bootable in QEMU.
#
#  QEMU denies a x86_64 multiboot elf file (GRUB2 accept it, btw):
#  https://github.com/qemu/qemu/blob/950c4e6c94b15cd0d8b63891dddd7a8dbf458e6a/hw/i386/multiboot.c#L197
#
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file")
    args = parser.parse_args()

    with open(args.file, 'r+b') as f:
        # Set EM_386 (0x0003) to em_machine.
        f.seek(18)
        f.write(bytes([0x03, 0x00]))

if __name__ == "__main__":
    main()
