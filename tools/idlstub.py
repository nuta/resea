#!/usr/bin/env python3
import argparse
from lark import Lark

def parse(source):
    parser = Lark("""

    """)

    ast = parser.parse(source)
    idl = {}
    return idl

def c_generator(idl):
    text = ""
    text += f"#ifndef __IDL_{idl['name']}_H__\n"
    text += f"#define __IDL_{idl['name']}_H__\n"
    text += "#endif\n"
    with open(args.out, "w") as f:
        f.write(generate(text))

def main():
    parser = argparse.ArgumentParser(description="The IDL stub generator.")
    parser.add_argument("--idl", required=True, help="The IDL file.")
    parser.add_argument("--lang", choices=["c"], default="c",
        help="The output language.")
    parser.add_argument("-o", dest="out", required=True,
        help="The output directory.")
    args = parser.parse_args()

    with open(args.idl) as f:
        idl = parse(f.read())

    if args.lang == "c":
        c_generator(idl)

if __name__ == "__main__":
    main()
