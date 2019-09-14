#!/usr/bin/env python3
import argparse
import os
import platform
import sys

def generate_default(args):
    cc = ""
    ld = ""
    llvm_prefix = ""
    grub_prefix = ""
    if platform.system() == "Darwin":
        grub_prefix += "/usr/local/opt/i386-elf-grub/bin/i386-elf-"
        llvm_prefix = "/usr/local/opt/llvm/bin/"

    # Use newer toolchain if possible (installed via apt.llvm.org).
    if os.path.exists("/usr/lib/llvm-9/bin"):
        llvm_prefix = "/usr/lib/llvm-9/bin/"
    if os.path.exists("/usr/lib/llvm-8/bin"):
        llvm_prefix = "/usr/lib/llvm-8/bin/"

    config = {
        "ARCH": "x64",
        "BUILD": "debug",
        "BUILD_DIR": "build",
        "INIT": "memmgr",
        "STARTUPS": "benchmark",
        "APPS": "",
        "PYTHON3": "python3",
        "LLVM_PREFIX": llvm_prefix,
        "GRUB_PREFIX": grub_prefix,
    }

    for user_var in args.vars:
        if "=" not in user_var:
            sys.exit("Variables must be formatted as 'FOO=bar'.")
        k,v = user_var.split("=", 1)
        config[k] = v

    config_mk = ""
    for key, value in config.items():
        config_mk += f"{key} := {value}\n"
    return config_mk

def main():
    parser = argparse.ArgumentParser(description="Generates the build config.")
    parser.add_argument("-o", dest="outfile", default=".config", help="The output file.")
    subparsers = parser.add_subparsers()
    default_parser = subparsers.add_parser("default", help="Generates default config.")
    default_parser.add_argument("vars", nargs="*", help="Config definitions (VAR_NAME=value).")
    default_parser.set_defaults(func=generate_default)
    args = parser.parse_args()    

    if "func" not in args:
        parser.print_help()
        sys.exit("")

    config = args.func(args)

    if os.path.exists(args.outfile) and config == open(args.outfile).read():
        # Don't update the build config file if its contents is not changed in
        # order to prevent make from rebuilding everything.
        return

    with open(args.outfile, "w") as f:
        f.write(config)

if __name__ == "__main__":
    main()
