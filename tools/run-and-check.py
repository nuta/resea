#!/usr/bin/env python3
import argparse
import subprocess
import sys
import signal
import os
from colorama import Fore, Back, Style

def scrub_stdout(stdout):
    return stdout.replace("\n\n", "\n") \
        .replace("\r", "") \
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
        p = subprocess.Popen(args.argv,
            preexec_fn=os.setsid,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        p.wait(args.timeout)
    except subprocess.TimeoutExpired as e:
        os.killpg(os.getpgid(p.pid), signal.SIGTERM)
        print(f"{Fore.YELLOW}{Style.BRIGHT}run-and-check.py: timed out:{Style.RESET_ALL}")

    stdout = scrub_stdout(p.stdout.read().decode("utf-8", "backslashreplace"))
    if args.expected not in stdout:
        print(stdout)
        print(f"{Fore.RED}{Style.BRIGHT}run-and-check.py: " +
            f"'{args.expected}' is not in stdout:{Style.RESET_ALL}")

        if not args.always_exit_with_zero:
            sys.exit(1)
    else:
        if not args.quiet:
            print(stdout)
            print(f"{Fore.GREEN}{Style.BRIGHT}run-and-check.py: OK{Style.RESET_ALL}")

if __name__ == "__main__":
    main()
