#!/usr/bin/env python3
import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser(
        description="Greps memory leaks in the log")
    parser.add_argument("--alloc-pattern",
                        default="@alloc.+ (?P<id>[a-zA-Z0-9]+)")
    parser.add_argument(
        "--free-pattern", default="@free.+ (?P<id>[a-zA-Z0-9]+)")
    parser.add_argument("log_file")
    args = parser.parse_args()

    leaks = set()
    for line in open(args.log_file).readlines():
        alloc_m = re.search(args.alloc_pattern, line)
        if alloc_m:
            leaks.add(alloc_m.groupdict()["id"])
        free_m = re.search(args.free_pattern, line)
        if free_m:
            id_ = free_m.groupdict()["id"]
            try:
                leaks.remove(id_)
            except KeyError:
                sys.stderr.write(f"{id_}: freed but allocated before\n")

    if len(leaks) == 0:
        print("No leaks found.")
        return

    for leak in leaks:
        print(leak)


if __name__ == "__main__":
    main()
