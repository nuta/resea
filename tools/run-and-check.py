#!/usr/bin/env python3
import argparse
import subprocess
import sys
from colorama import Fore, Back, Style

def scrub_stdout(stdout):
    return stdout.replace("\n\n", "\n") \
        .replace("\x1b\x63", "") \
        .replace("\x1b[2J", "").strip()

def main():
    parser = argparse.ArgumentParser(
        description="Run a command and check its output contains an expected string.")
    parser.add_argument("--timeout", type=int, default=16)
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--always-exit-with-zero", action="store_true")
    parser.add_argument("expected", help="The expected string in the output.")
    parser.add_argument("argv", nargs="+", help="The command.")
    args = parser.parse_args()

    try:
        p = subprocess.run(args.argv, timeout=args.timeout, universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired as e:
        stdout = scrub_stdout(e.stdout.decode("utf-8", "backslashreplace"))
        print(f"{Fore.RED}{Style.BRIGHT}run-and-check.py: timed out:{Style.RESET_ALL}\n{stdout}")

    stdout = scrub_stdout(p.stdout)
    if args.expected not in stdout:
        print(stdout)
        print(f"{Fore.RED}{Style.BRIGHT}run-and-check.py: " +
            f"'{args.expected}' is not in stdout:{Style.RESET_ALL}\n{stdout}")

        if not args.always_exit_with_zero:
            sys.exit(1)
    else:
        if not args.quiet:
            print(stdout)
            print(f"{Fore.GREEN}{Style.BRIGHT}run-and-check.py: OK{Style.RESET_ALL}")

if __name__ == "__main__":
    main()
