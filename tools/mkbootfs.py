#!/usr/bin/env python3
import argparse
from pathlib import Path
import struct
import sys

VERSION = 1
PAGE_SIZE = 4096
JUMP_CODE_SIZE = 16
FILE_ENTRY_SIZE = 64
BOOTFS_MAX_SIZE = 64 * 1024 * 1024


def align_up(value, align):
    return (value + align - 1) & ~(align - 1)


def main():
    parser = argparse.ArgumentParser(description="Builds a bootfs.")
    parser.add_argument("-o", dest="output", help="The output file.")
    parser.add_argument("files", nargs="*", help="Files.")
    args = parser.parse_args()

    # Write the boot binary and the file system header.
    bootfs = struct.pack("IIII", VERSION, 16, len(args.files), 0)

    # Append files.
    file_contents = bytes()
    file_off = align_up(len(bootfs) + FILE_ENTRY_SIZE *
                        len(args.files), PAGE_SIZE)
    for path in args.files:
        name = str(Path(path).stem)
        if len(name) >= 48:
            sys.exit(f"too long file name: {name}")

        data = open(path, "rb").read()
        file_contents += data
        bootfs += struct.pack("48sII8x",
                              name.encode("ascii"), file_off, len(data))

        padding = align_up(len(file_contents), PAGE_SIZE) - len(file_contents)
        file_contents += struct.pack(f"{padding}x")
        file_off += len(data) + padding

    padding = align_up(len(bootfs), PAGE_SIZE) - len(bootfs)
    bootfs += struct.pack(f"{padding}x")
    bootfs += file_contents

    if len(bootfs) > BOOTFS_MAX_SIZE:
        sys.exit(f"bootfs.bin is too big ({len(bootfs) / 1024}KB)")

    with open(args.output, "wb") as f:
        f.write(bootfs)


if __name__ == "__main__":
    main()
