#!/usr/bin/env python3
#
#  Usage:
#
#    $ make RUSTFLAGS="-Z emit-stack-sizes" kernel.elf
#    $ ./tools/analyze-stack-usage.py kernel.elf | sort -n
#
import argparse
import subprocess
import tempfile
import struct
import sys

def parse_leb128(buf):
    value = 0
    i = 0
    while True:
        b = buf[i]
        value |= (b & 0x7f) << (7 * i)
        if b & 0x80 == 0:
            return value, i + 1
        i += 1

def main():
    parser = argparse.ArgumentParser(description="The static stack usage analyzer.")
    parser.add_argument("elf_file")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tempdir:
        temp_file = tempdir + "/temp.bin"
        stack_file = tempdir + "/stack_sizes.bin"

        subprocess.run(["objcopy", "-Obinary", "--dump-section", f".stack_sizes={stack_file}",
            args.elf_file, temp_file])

        nm_output = subprocess.check_output(["nm", "--demangle", args.elf_file], text=True)
        functions = {}
        for line in nm_output.splitlines():
            cols = line.split(" ", 2)
            functions[int(cols[0], 16)] = cols[2]

        stack_sizes = open(stack_file, "rb").read()
        if len(stack_sizes) == 0:
            sys.exit("error: size of .stack_sizes is 0")

        index = 0
        while index < len(stack_sizes):
            addr = struct.unpack("Q", stack_sizes[index:index+8])[0]
            size, size_len = parse_leb128(stack_sizes[index+8:])
            if addr in functions:
                print(f"{size}\t{functions[addr]}")

            index += 8 + size_len

if __name__ == "__main__":
    main()