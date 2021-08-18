#!/usr/bin/env python3
import argparse
from pathlib import Path
import struct
import sys

MAGIC = 0xaa210001
PAGE_SIZE = 4096

def align_up(value, align):
    return (value + align - 1) & ~(align - 1)


def main():
    parser = argparse.ArgumentParser(description="Builds a assetfs.")
    parser.add_argument("-o", dest="output", help="The output file.")
    parser.add_argument("dir", help="The asset directory.")
    args = parser.parse_args()

    entries = bytes()
    file_contents = bytes()
    file_off = 16 # size of the fs header
    num_entries = 0
    for path in Path(args.dir).glob("*"):
        name = str(Path(path).stem)
        if len(name) >= 48:
            sys.exit(f"too long file name: {name}")

        data = open(path, "rb").read()
        file_contents += data
        entries += struct.pack("48sII8x",
                              name.encode("ascii"), file_off, len(data))

        padding = align_up(len(file_contents), 16) - len(file_contents)
        file_contents += struct.pack(f"{padding}x")
        file_off += len(data) + padding
        num_entries += 1

    image = struct.pack("II8x", MAGIC, num_entries) + entries + file_contents

    with open(args.output, "wb") as f:
        f.write(image)

if __name__ == "__main__":
    main()
