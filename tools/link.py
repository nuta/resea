#!/usr/bin/env python3
import argparse
import subprocess
import os
import sys
import shlex
import shutil

def extract_symbols(nm, file):
    stdout = subprocess.check_output([nm, "--defined-only", "--demangle", file],
        encoding="utf-8")
    symbols = {}
    symtable_addr = None
    for line in stdout.strip().split("\n"):
        cols = line.split(" ", 2)
        addr = int(cols[0], 16)
        name = cols[2]
        symbols[addr] = name
        if name == "__symtable":
            symtable_addr = addr
    assert symtable_addr
    return symbols, symtable_addr

def generate_symbol_table(outfile, symbols, symtable_addr):
    symbols = sorted(symbols.items(), key=lambda s: s[0])
    symtable = f"""\
#ifdef __x86_64__
#define PTR      .quad
#define PTRSIZE  8
#else
#error \"Unexpected architecture. Update gen-symtable.py!\"
#endif
#define HEADER_SIZE   (8 + PTRSIZE * 2)
#define SIZEOF_SYMBOL (PTRSIZE + 4 * 2)

.section .rodata
.globl __symtable
__symtable:
    .long   0x2b012b01       // magic
    .short  {len(symbols)}   // num_symbols
    .short  0                // reserved
    PTR     ({hex(symtable_addr)} + HEADER_SIZE)  // symbols
    PTR     ({hex(symtable_addr)} + HEADER_SIZE + SIZEOF_SYMBOL * {len(symbols)}) // strbuf
"""
    
    symtable += "// struct symbol symbols[]\n"
    offset = 0
    for addr, name in symbols:
        symtable += f"""\
PTR     {hex(addr)}  // addr
.short  {offset}     // offset
.space  6            // reserved
"""
        offset += len(name) + 1

    symtable += "// strings\n"
    offset = 0
    for _, name in symbols:
        symtable += f".asciz \"{name}\"\n"

    with open(outfile, "w") as f:
        f.write(symtable)

def swap_key_and_value(d):
    return { v: k for k, v in d.items() }

def verify_symtable(nm, old_file, new_file):
    symbols = swap_key_and_value(extract_symbols(nm, old_file)[0])
    new_symbols = swap_key_and_value(extract_symbols(nm, new_file)[0])
    for name in symbols:
        if symbols[name] != new_symbols[name]:
            sys.exit(f"Incorrect symbol table entry '{name}': " +
                f"expected={symbols[name]}, actual={new_symbols[name]}")

def link(cc, cflags, ld, ldflags, nm, build_dir, outfile, mapfile, objs):
    # Link stage 1: Embed an empty symtable into the executable.
    generate_symbol_table(f"{build_dir}/__symtable.S", {}, 0)
    subprocess.check_call([cc] + shlex.split(cflags) + \
        ["-c", "-o", f"{build_dir}/__symtable.o", f"{build_dir}/__symtable.S"])
    subprocess.check_call([ld] + shlex.split(ldflags) + \
        ["-o", f"{outfile}.stage1.tmp", f"{build_dir}/__symtable.o"] + objs)
    
	# Link stage 2: Embed the generated symtable into the executable.
	#               Symbol offsets may be changed (that is, the symtable is
	#               not correct) in this stage since the size of the
	#               symtable is no longer empty.
    generate_symbol_table(f"{build_dir}/__symtable.S", *extract_symbols(nm, f"{outfile}.stage1.tmp"))
    subprocess.check_call([cc] + shlex.split(cflags) + \
        ["-c", "-o", f"{build_dir}/__symtable.o", f"{build_dir}/__symtable.S"])
    subprocess.check_call([ld] + shlex.split(ldflags) + \
        ["-o", f"{outfile}.stage2.tmp", f"{build_dir}/__symtable.o"] + objs)
    
	# Link stage 3: Embed re-generated symtable into the executable again
	#               because symbol offsets may be changed by embedding the
	#               symtable in the stage 2. Here we assume that the linker
	#               behaves in a deterministic way, i.e., symbol offsets in
	#               the re-generated symtable and in the $@.stage3.tmp are
	#               identical.
    generate_symbol_table(f"{build_dir}/__symtable.S", *extract_symbols(nm, f"{outfile}.stage2.tmp"))
    subprocess.check_call([cc] + shlex.split(cflags) + \
        ["-c", "-o", f"{build_dir}/__symtable.o", f"{build_dir}/__symtable.S"])
    subprocess.check_call([ld, "--Map", mapfile, "-o", f"{outfile}.stage3.tmp"] \
        + shlex.split(ldflags) + [f"{build_dir}/__symtable.o"] + objs)

    verify_symtable(nm, f"{outfile}.stage2.tmp", f"{outfile}.stage3.tmp")
    shutil.move(f"{outfile}.stage3.tmp", outfile)

def main():
    parser = argparse.ArgumentParser(
        description="Link and an symbol table.")
    parser.add_argument("--cc", help="The clang path.", required=True)
    parser.add_argument("--cflags", help="The clang options.", required=True)
    parser.add_argument("--ld", help="The ld path.", required=True)
    parser.add_argument("--ldflags", help="The ld options.", required=True)
    parser.add_argument("--nm", help="The nm path.", required=True)
    parser.add_argument("--build-dir", help="The build directory.", required=True)
    parser.add_argument("--mapfile", help="The map file.", required=True)
    parser.add_argument("--outfile", help="The output file.", required=True)
    parser.add_argument("objs", help="The object files.", nargs="+")
    args = parser.parse_args()

    link(args.cc, args.cflags, args.ld, args.ldflags, args.nm, args.build_dir,
        args.outfile, args.mapfile, args.objs)

if __name__ == "__main__":
    main()
