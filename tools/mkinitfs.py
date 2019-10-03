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

# Don't change this value: the kernel memory map layout assumes that initfs.bin
# is smaller than 16MiB!
INITFS_MAX_SIZE = (16 *1024 * 1024) - 1

def construct_file_header(name, data, length):
    assert len(str(name)) < 48
    padding = PAGE_SIZE - (length % PAGE_SIZE)
    header = struct.pack("48sIH10x", str(name).encode("utf-8"), length, padding)
    return header, padding

def make_image(startup, root_dir, file_list):
    image = bytes()

    # Append the startup code.
    num_files = 1
    startup_bin = open(startup, "rb") .read()
    startup_len = len(startup_bin) - FS_HEADER_SIZE - FILE_HEADER_SIZE
    header, padding = construct_file_header(Path(startup).name, startup_bin,
        startup_len)
    image += startup_bin + bytearray(padding)
    image = image[:FS_HEADER_SIZE] + header + image[FS_HEADER_SIZE+len(header):]

    # Append files.
    for file in Path(root_dir).glob("**/*"):
        if not file.is_file():
            continue

        name = file.relative_to(root_dir)
        if file_list and str(name) not in file_list:
            print(f"mkinitfs: Excluding '{name}'")
            continue

        if len(str(name)) >= 32:
            sys.exit(f"too long file name: {name}")

        data = open(file, "rb").read()
        header, padding = construct_file_header(name, data, len(data))
        image += header + data + struct.pack(f"{padding}x")
        num_files += 1

    # Write the file system header.
    fs_header = struct.pack("III", VERSION, len(image), num_files)
    image = image[:JUMP_CODE_SIZE] + fs_header + \
        image[JUMP_CODE_SIZE+len(fs_header):]

    if len(image) > INITFS_MAX_SIZE:
        sys.exit(f"initfs.bin is too big ({len(image) / 1024}KiB)")
    return image

def generate_asm(binfile):
    return f"""\
.globl __initfs
.align 4096
__initfs:
    .incbin "{binfile}"
"""

def main():
    parser = argparse.ArgumentParser(description="Generates a initfs image.")
    parser.add_argument("-o", dest="output", help="The output file.")
    parser.add_argument("-s", dest="startup",
        help="The init server executable.")
    parser.add_argument("--file-list", help="Files to include.")
    parser.add_argument("--generate-asm",
        help="Generate an assembly file to embed initfs.")
    parser.add_argument("dir",
        help="The root directory of initfs to be embeddded.")
    args = parser.parse_args()

    with open(args.output, "wb") as f:
        f.write(make_image(args.startup, args.dir, args.file_list))

    if args.generate_asm:
        with open(args.generate_asm, "w") as f:
            f.write(generate_asm(args.output))

if __name__ == "__main__":
    main()
