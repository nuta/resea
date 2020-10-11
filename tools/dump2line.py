#!/usr/bin/env python3
import argparse
import subprocess
import sys
import re

def main():
    parser = argparse.ArgumentParser(
        description="Backtraces a QEMU memory dump (e.g. x /100g $esp)")
    parser.add_argument("--addr2line", default="gaddr2line", help="addr2line")
    parser.add_argument("--image", default="build/resea.elf",
        help="The executable.")
    args = parser.parse_args()

    for line in sys.stdin.readlines():
        m = re.match(r"^([0-9a-f]+):", line)
        if m:
            _, addrs = line.split(": ")
            for addr in addrs.split(" "):
                addr = addr.strip()
                stdout = subprocess.check_output(
                    [args.addr2line, "-e", args.image, addr]) \
                        .decode("utf-8").strip()
                if stdout.startswith("??"):
                    continue
                print(f"{addr} {stdout}")

if __name__ == "__main__":
    main()
