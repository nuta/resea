#!/usr/bin/env python3
import argparse
import subprocess

def trace(executable, body):
    addrs = []
    for l in body.split("\n"):
        cols = l.split(" ")
        try:
            addr1 = cols[2] + cols[1][2:]
            addr2 = cols[4] + cols[3][2:]
            addrs.append(addr1)
            addrs.append(addr2)
        except IndexError:
            pass

    i = 0
    addrs = list(filter(lambda a: int(a, 16) > 0x1000, addrs))
    for addr in addrs:
        stdout = subprocess.check_output(
            ["/usr/local/opt/binutils/bin/gaddr2line", "-e", executable, addr]
        ).decode("utf-8").strip()

        if not stdout.startswith("??:"):
            print(f"#{i}: {stdout}")
            i += 1

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable")
    parser.add_argument("trace")
    args = parser.parse_args()

    with open(args.trace) as f:
        trace(args.executable, f.read())


if __name__ == "__main__":
    main()
