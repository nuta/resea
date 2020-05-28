#!/usr/bin/env python3
import argparse
import struct

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    parser.add_argument("elf_file")
    args = parser.parse_args()

    # Embed the server name into the padding space (EI_PAD).
    e_ident = open(args.elf_file, "rb").read(7) + \
        struct.pack("9s", args.name.encode("ascii"))
    assert len(e_ident) == 16
    with open(args.elf_file, "r+b") as f:
        f.write(e_ident)

if __name__ == "__main__":
    main()
