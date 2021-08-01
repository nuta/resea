#!/usr/bin/env python3
import argparse
import os
import sys
from pathlib import Path
from colorama import Fore, Style


def error(msg):
    sys.exit(
        f"{Fore.RED}{Style.BRIGHT}scan-libs-dir.py: {msg}{Style.RESET_ALL}")


def lib_dir_lint(lib_dir: Path):
    lib_name = lib_dir.stem
    makefile = open(lib_dir / "build.mk").read()
    if f"name:={lib_name}" not in makefile.replace(" ", ""):
        error(f"The correct lib name definition (name := {lib_name}) "
              "not found in {lib_dir}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--names", action="store_true")
    parser.add_argument("--dir")
    args = parser.parse_args()

    libs = {}
    for (lib_dir, dirs, files) in os.walk("libs"):
        if "build.mk" not in files:
            continue

        lib_dir = Path(lib_dir)
        if any([(d / "build.mk").exists() for d in lib_dir.parents]):
            continue

        lib_dir_lint(lib_dir)
        libs[lib_dir.stem] = lib_dir

    if args.names:
        print(" ".join(libs.keys()))

    if args.dir:
        lib_dir = libs[args.dir]
        if not lib_dir:
            error(f"No such a library: '{args.dir}'")
        print(lib_dir)


if __name__ == "__main__":
    main()
