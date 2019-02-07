#!/usr/bin/env python3
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser(
        description="Monitor file system changes and automatically run a command.")
    parser.add_argument("command")
    args = parser.parse_args()

    subprocess.run(args.command, shell=True)
    subprocess.run(["watchmedo", "shell-command", "--patterns=*.rs;*.c;*.S;*.toml;*.py;*.sh;*.mk;*.json",
        "--recursive", "--command", args.command])

if __name__ == "__main__":
    main()