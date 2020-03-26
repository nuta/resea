#!/usr/bin/env python3
import argparse
from pathlib import Path
import struct
import sys

VERSION = 1
PAGE_SIZE = 4096
JUMP_CODE_SIZE = 16
FILE_ENTRY_SIZE = 64
INITFS_MAX_SIZE = 8 *1024 * 1024

def align_up(value, align):
    return (value + align - 1) & ~(align - 1)

def main():
    parser = argparse.ArgumentParser(description="Generates a initfs initfs.")
    parser.add_argument("-o", dest="output", help="The output file.")
    parser.add_argument("--init", dest="init", help="The init binary.")
    parser.add_argument("files", nargs="*", help="Files.")
    args = parser.parse_args()

    # Write the init binary and the file system header.
    init_bin = open(args.init, "rb").read()
    header = struct.pack("IIII", VERSION, len(init_bin), len(args.files), 0)
    initfs = init_bin[:JUMP_CODE_SIZE] + header + init_bin[JUMP_CODE_SIZE+len(header):]

    # Append files.
    file_contents = bytes()
    file_off = align_up(len(initfs) + FILE_ENTRY_SIZE * len(args.files), PAGE_SIZE)
    for path in args.files:
        name = str(Path(path).stem)
        if len(name) >= 48:
            sys.exit(f"too long file name: {name}")

        data = open(path, "rb").read()
        file_contents += data
        initfs += struct.pack("48sII8x", name.encode("ascii"), file_off, len(data))

        padding = align_up(len(file_contents), PAGE_SIZE) - len(file_contents)
        file_contents += struct.pack(f"{padding}x")
        file_off += len(data) + padding

    padding = align_up(len(initfs), PAGE_SIZE) - len(initfs)    
    initfs += struct.pack(f"{padding}x")
    initfs += file_contents

    if len(initfs) > INITFS_MAX_SIZE:
        sys.exit(f"initfs.bin is too big ({len(initfs) / 1024}KB)")

    with open(args.output, "wb") as f:
        f.write(initfs)

if __name__ == "__main__":
    main()
