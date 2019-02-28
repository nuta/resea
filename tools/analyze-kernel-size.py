#!/usr/bin/env python3
import argparse
import subprocess
import colorama
from colorama import Fore, Style

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("image")
    args = parser.parse_args()

    stdout = subprocess.check_output(["greadelf", "-W", "-s", args.image]).decode("utf-8")
    symbols = {}
    total = 0
    for line in stdout.split("\n"):
        cols = list(filter(lambda w: w != "", line.split(" ")))
        if len(cols) < 8:
            continue

        try:
            size = int(cols[2])
        except ValueError:
            continue

        ident = subprocess.check_output(["c++filt"], input=cols[7].encode("utf-8")).decode("utf-8")

        if size > 0:
            symbols[ident] = size
            total += size

    print(f"{Style.BRIGHT}{Fore.GREEN}{Fore.RESET}10 Most Largest Parts ================================{Style.RESET_ALL}")
    for ident, size in sorted(symbols.items(), key=lambda x: x[1], reverse=True)[:10]:
        print(size, ident)

if __name__ == "__main__":
    colorama.init(autoreset=True)
    main()
