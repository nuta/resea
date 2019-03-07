#!/usr/bin/env python3
import argparse
import atexit
import re
import subprocess
import sys
import colorama
from colorama import Fore, Style

def prettify_kernel_message(line):
    if line.startswith("[PANIC]"):
        print(f"{Style.BRIGHT}{Fore.RED}{line}{Fore.RESET}")
    elif line.startswith("[Oops]"):
        print(f"{Style.BRIGHT}{Fore.YELLOW}{line}{Fore.RESET}")
    elif line.startswith("WARN"):
        print(f"{Style.BRIGHT}{Fore.MAGENTA}{line}{Fore.RESET}")
    elif re.match(r"[ ]*#\d+: ", line):
        # Possibly a stack trace.
        print(f"{Style.BRIGHT}{Fore.YELLOW}{line}{Fore.RESET}")
    else:
        print(line)

def atexit_handler():
    # Kill all child processes.
    qemu_proc.kill()
    qemu_proc.wait()

def main():
    global qemu_proc
    parser = argparse.ArgumentParser(description="Run QEMU and prettify kernel messages.")
    parser.add_argument("qemu", nargs="+")
    args = parser.parse_args()

    # Ensure that child processes will be killed.
    atexit.register(atexit_handler)

    print(Style.BRIGHT + Fore.GREEN + "==> " + Fore.RESET + "Lauching QEMU")
    qemu_proc = subprocess.Popen(args.qemu, stdout=subprocess.PIPE,)

    line = ""
    in_esc_seq = False
    esc_seq = ""
    in_bios = True
    qemu_prompt = False
    while True:
        ch = qemu_proc.stdout.read(1).decode("ascii")
        if len(ch) == 0:
            if qemu_proc.poll() is not None:
                print("\n" + Style.BRIGHT + Fore.GREEN + "==> " + Fore.RESET + "QEMU has been exited")
                return

        if ch == "\n":
            if in_bios:
                if "Booting from ROM.." in line:
                    in_bios = False
                    first_line = line.replace("Booting from ROM..", "")
                    print(first_line)
            else:
                prettify_kernel_message(line)
            line = ""
        # The beginning of a line.
        elif len(line) == 0:
            # The first character of the prompt `(qemu) '.
            qemu_prompt = ch == "("

            line += ch
            if qemu_prompt:
                print(ch, end="")
        else:
            line += ch
            if qemu_prompt:
                if in_esc_seq:
                    if ch.isalpha():
                        # The end of an escape sequence.
                        esc_seq += ch
                        line += esc_seq
                        in_esc_seq = False
                        print(esc_seq, end="")
                    else:
                        esc_seq += ch
                # Is ESC?
                elif ord(ch) == 27:
                    # If so, we have to buffer the escape sequence since terminal
                    # emulators seem to ignore incomplete sequences.
                    in_esc_seq = True
                    esc_seq = ch
                else:
                    print(ch, end="")
        sys.stdout.flush()

if __name__ == "__main__":
    colorama.init(autoreset=True)
    main()
