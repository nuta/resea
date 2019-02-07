#!/usr/bin/env python3
import argparse
import atexit
import subprocess
import colorama
from colorama import Fore, Style

def prettify_kernel_message(line):
    pass

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
    is_kernel_message = False
    while True:
        ch = qemu_proc.stdout.read(1).decode("ascii")
        if len(ch) == 0:
            if qemu_proc.poll() is not None:
                print("\n" + Style.BRIGHT + Fore.GREEN + "==> " + Fore.RESET + "QEMU has been exited")
                return

        if ch == "\n":
            if is_kernel_message:
                prettify_kernel_message(line)
            elif in_bios:
                if "Booting from ROM.." in line:
                    in_bios = False
                    first_line = line.replace("Booting from ROM..", "")
                    print(first_line)
            else:
                print()

            line = ""
        # The beginning of a line.
        elif len(line) == 0:
            if ch == ">":
                # The current is likely a kernel message.
                is_kernel_message = True
            else:
                is_kernel_message = False

            line += ch
            if not in_bios and not is_kernel_message:
                print(ch, end="")
        else:
            line += ch
            if not in_bios and not is_kernel_message:
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

if __name__ == "__main__":
    colorama.init(autoreset=True)
    main()