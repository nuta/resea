#!/usr/bin/env python3
import argparse
from pathlib import Path
import struct
import sys

VERSION = 1
PAGE_SIZE = 4096
JUMP_CODE_SIZE = 16
FS_HEADER_SIZE = 128
FILE_HEADER_SIZE = 64

def construct_file_header(name, data, length):
    assert len(str(name)) < 48
    padding = PAGE_SIZE - (length % PAGE_SIZE)
    header = struct.pack("48sIH10x", str(name).encode("utf-8"), length, padding)
    return header, padding

def make_image(startup, root_dir):
    image = bytes()

    # Append the startup code.
    num_files = 1
    startup_bin = open(startup, "rb") .read()
    startup_len = len(startup_bin) - FS_HEADER_SIZE - FILE_HEADER_SIZE
    header, padding = construct_file_header(Path(startup).name, startup_bin, startup_len)
    image += startup_bin + bytearray(padding)
    image = image[:FS_HEADER_SIZE] + header + image[FS_HEADER_SIZE+len(header):]

    # Append files.
    for file in Path(root_dir).glob("**/*"):
        if not file.is_file():
            continue

        name = file.relative_to(root_dir)
        data = open(file, "rb").read()

        if len(str(name)) >= 32:
            sys.exit(f"too long file name: {name}")

        header, padding = construct_file_header(name, data, len(data))
        image += header + data + struct.pack(f"{padding}x")
        num_files += 1

    # Write the file system header.
    fs_header = struct.pack("III", VERSION, len(image), num_files)
    image = image[:JUMP_CODE_SIZE] + fs_header + image[JUMP_CODE_SIZE+len(fs_header):]

    return image

def generate_asm(binfile):
    return f"""\
.globl __initfs
.align 4096
__initfs:
    .incbin "{binfile}"
"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", dest="output")
    parser.add_argument("-s", dest="startup")
    parser.add_argument("--generate-asm")
    parser.add_argument("dir")
    args = parser.parse_args()

    with open(args.output, "wb") as f:
        f.write(make_image(args.startup, args.dir))

    if args.generate_asm:
        with open(args.generate_asm, "w") as f:
            f.write(generate_asm(args.output))

if __name__ == "__main__":
    main()
