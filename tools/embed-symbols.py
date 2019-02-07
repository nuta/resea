#!/usr/bin/env python3
import argparse
import subprocess
import struct
import sys

def extract_symbols(nm, file):
    stdout = subprocess.check_output([nm, "--defined-only", "--demangle", file],
        encoding="utf-8")
    symbols = {}
    table_offset = None
    for line in stdout.strip().split("\n"):
        cols = line.split(" ", 2)
        addr = int(cols[0], 16)
        name = cols[2]
        symbols[addr] = name
        if name == "__symbols":
            table_offset = addr
    return symbols, table_offset

def generate_symbol_table(symbols):
    num_symbols = len(symbols)
    addr_array = bytes()
    strbuf = bytes()
    str_offset = 0
    for addr in sorted(symbols.keys()):
        name = symbols[addr]
        addr_array += struct.pack("<QLL", addr, str_offset, len(name))
        strbuf += bytes(name, "ascii")
        str_offset += len(name)

    magic = b"SYMBOLS!"
    strbuf_offset = len(addr_array)
    header = magic + struct.pack("<QQQ", num_symbols, strbuf_offset, len(strbuf))
    table = header + addr_array + strbuf
    return table

def main():
    parser = argparse.ArgumentParser(description="embeds symbol table")
    parser.add_argument("image", help="The kernel image file.")
    parser.add_argument("--dump", help="Dump the table to a file instead.")
    parser.add_argument("--nm", default="nm", help="The nm command (should be GNU binutils one).")
    args = parser.parse_args()

    symbols, table_offset = extract_symbols(args.nm, args.image)
    table = generate_symbol_table(symbols)

    if args.dump is None:
        # Embed the symbol table.
        with open(args.image, "rb") as f:
            data = f.read()

        table_offset = data.find(b"__SYMBOL_TABLE_START__")
        table_end = data.find(b"__SYMBOL_TABLE_END__")
        if table_offset == -1 or table_end == -1:
            sys.exit("failed to locate __symbols")

        available_size = table_end - table_offset
        if len(table) > available_size:
            sys.exit("The symbol table space is too small.")

        pad = bytes(" " * (available_size - len(table)), "ascii")
        embedded = data[:table_offset] + table + pad + data[table_offset + available_size:]
        with open(args.image, "wb") as f:
            f.write(embedded)
    else:
        with open(args.dump, "wb") as f:
            f.write(table)


if __name__ == "__main__":
    main()