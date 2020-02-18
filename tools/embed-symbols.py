#!/usr/bin/env python3
import argparse
import sys
import struct

SYMBOL_TABLE_EMPTY = bytes([0x53, 0x59, 0x4d, 0xb0, 0x53, 0x59, 0x4d, 0xb0])
SYMBOL_TABLE_MAGIC = bytes([0x53, 0x59, 0x4d, 0x4c, 0x53, 0x59, 0x4d, 0x4c])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("symbols_file")
    parser.add_argument("executable")
    args = parser.parse_args()

    # Locate the symbol table in the executable.
    image = open(args.executable, "rb").read()
    offset = image.find(SYMBOL_TABLE_EMPTY)
    if offset < 0:
        sys.exit("failed to locate the symbol table")
    if image.find(SYMBOL_TABLE_EMPTY, offset + 1) >= 0:
        sys.exit("found multiple empty symbol tables (perhaps because " +
            "SYMBOL_TABLE_EMPTY is not sufficiently long to be unique?)")

    # Extract the SYMBOLS_MAX value.
    _, num_symbols_max = struct.unpack("<QI", image[offset:offset+12])

    # Parse the nm output and extract symbol names and theier addresses.
    symbols = {}
    for line in open(args.symbols_file).read().strip().split("\n"):
        cols = line.split(" ", 1)
        addr = int(cols[0], 16)
        name = cols[1]
        symbols[addr] = name

    symbols = sorted(symbols.items(), key=lambda s: s[0])

    # Build a symbol table.
    symbol_table = struct.pack("<8sII", SYMBOL_TABLE_MAGIC, len(symbols), 0)
    for addr, name in symbols:
        symbol_table += struct.pack("<Q56s", addr, bytes(name[:55], "ascii"))
    for _ in range(len(symbols), num_symbols_max):
        symbol_table += struct.pack("<Q56s", 0, b"")

    # Embed the symbol table.
    image = image[:offset] + symbol_table + image[offset + len(symbol_table):]
    open(args.executable, "wb").write(image)

if __name__ == "__main__":
    main()
