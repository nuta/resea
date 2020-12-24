#!/usr/bin/env python3
import argparse
import subprocess
import sys
import re


def main():
    parser = argparse.ArgumentParser(
        description="Try applying adddr2line to arbitrary hex strings in stdin.")
    parser.add_argument("--addr2line", default="gaddr2line", help="addr2line")
    parser.add_argument("--image", default="build/resea.elf",
                        help="The executable.")
    args = parser.parse_args()

    for line in sys.stdin.readlines():
        for addr in re.findall(r"\W(?P<addr>[0-9a-f]{8,})\W", line):
            addr = addr.strip()
            stdout = subprocess.check_output(
                [args.addr2line, "-e", args.image, addr]) \
                .decode("utf-8").strip()
            print(f"{addr} {stdout}")
            if stdout.startswith("??"):
                continue


if __name__ == "__main__":
    main()
