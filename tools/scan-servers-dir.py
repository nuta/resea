#!/usr/bin/env python3
import argparse
import os
import sys
from pathlib import Path
from colorama import Fore, Back, Style

def error(msg):
    sys.exit(f"{Fore.RED}{Style.BRIGHT}scan-servers-dir.py: {msg}{Style.RESET_ALL}")

def server_dir_lint(server_dir: Path):
    server_name = server_dir.stem
    makefile = open(server_dir / "build.mk").read()
    if f"name:={server_name}" not in makefile.replace(" ", ""):
        error(f"The correct server name definition (name := {server_name}) "
            "not found in {server_dir}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--names", action="store_true")
    parser.add_argument("--dir")
    args = parser.parse_args()

    servers = {}
    for (server_dir, dirs, files) in os.walk("servers"):
        if "build.mk" not in files:
            continue

        server_dir = Path(server_dir)
        if any([(d / "build.mk").exists() for d in server_dir.parents]):
            continue

        server_dir_lint(server_dir)
        servers[server_dir.stem] = server_dir

    if args.names:
        print(" ".join(servers.keys()))

    if args.dir:
        server_dir = servers[args.dir]
        if not server_dir:
            error(f"No such a server: '{args.dir}'")
        print(server_dir)

if __name__ == "__main__":
    main()

