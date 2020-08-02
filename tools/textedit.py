#!/usr/bin/env python3
import argparse
import sys
import re

def main():
    parser = argparse.ArgumentParser(
        description="A cross-platform text editing tool like egrep(1), sed(1), etc.")
    parser.add_argument("-f", "--filter", action="append", default=[])
    parser.add_argument("-r", "--replace", nargs=2, action="append", default=[])
    parser.add_argument("-u", "--uppercase", action="store_true")
    args = parser.parse_args()

    lines = []
    for line in sys.stdin:
        line = line.rstrip("\n")
        if not args.filter or any(map(lambda pat: re.search(pat, line), args.filter)):
            lines.append(line)

    text = "\n".join(lines)
    for (old, new) in args.replace:
        text = re.sub(old, new, text)

    if args.uppercase:
        text = text.upper()

    print(text)

if __name__ == "__main__":
    main()
