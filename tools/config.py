#!/usr/bin/env python3
import argparse
import defconfig
import menuconfig
import kconfiglib
import re
import platform
import os
import sys
from glob import glob
from pathlib import Path

def defconfig(config_file):
    # Search for toolchains.
    llvm_prefix = ""
    grub_prefix = ""
    if platform.system() == "Darwin":
        grub_prefix += "/usr/local/opt/i386-elf-grub/bin/i386-elf-"
        llvm_prefix = "/usr/local/opt/llvm/bin/"

    # Use newer toolchain if possible (installed via apt.llvm.org).
    if os.path.exists("/usr/lib/llvm-9/bin"):
        llvm_prefix = "/usr/lib/llvm-9/bin/"
    elif os.path.exists("/usr/lib/llvm-8/bin"):
        llvm_prefix = "/usr/lib/llvm-8/bin/"

    os.environ["DEFAULT_LLVM_PREFIX"] = llvm_prefix
    os.environ["DEFAULT_GRUB_PREFIX"] = grub_prefix

    if os.path.exists(config_file):
        os.remove(config_file)
    kconf = kconfiglib.Kconfig("Kconfig")
    kconf.load_config()
    kconf.write_config()

    generate_mk(config_file)

def generate_mk(config_file):
    config_mk = open(config_file).read()

    # XXX:
    config_mk += "\n"
    if "CONFIG_ARCH_X64=y" in config_mk:
        config_mk += "CONFIG_ARCH=x64\n"
    if "CONFIG_BUILD_RELEASE=y" in config_mk:
        config_mk += "BUILD=release\n"

    servers = []
    for server in glob("servers/*"):
        name = Path(server).name
        if f"CONFIG_{name.upper()}_SERVER=y" in config_mk:
            servers.append(name)
    config_mk += "SERVERS := " + " ".join(servers) + "\n"

    config_mk = re.sub(r"(^|\n)CONFIG_([^=]+)=\"?([^\"\n]*)\"?", r"\n\2:=\3", config_mk)
    open(config_file + ".mk", "w").write(config_mk)

def generate_h(config_file, outfile):
    config_mk = open(config_file).read()
    config_h = re.sub(r"(^|\n)(CONFIG_[^=]+)=(.*)", r"\n#define \2 \3", config_mk)
    open(outfile, "w").write(config_h)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--menuconfig", action="store_true")
    parser.add_argument("--default", action="store_true")
    parser.add_argument("--generate", metavar="OUTFILE")
    parser.add_argument("--config-file", default=".config")
    args = parser.parse_args()

    if args.menuconfig:
        sys.argv = sys.argv[:1]
        menuconfig._main()
        generate_mk(args.config_file)
    elif args.default:
        defconfig(args.config_file)
    elif args.generate:
        generate_h(args.config_file, args.generate)

if __name__ == "__main__":
    main()
