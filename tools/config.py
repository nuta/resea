#!/usr/bin/env python3
import argparse
import menuconfig
import kconfiglib
import os
import sys


def defconfig(config_file):
    if os.path.exists(config_file):
        os.remove(config_file)
    kconf = kconfiglib.Kconfig("Kconfig")
    if os.path.exists(config_file):
        kconf.load_config(filename=config_file)
    kconf.write_config(filename=config_file)


def genconfig(config_file, outfile):
    kconf = kconfiglib.Kconfig("Kconfig")
    kconf.load_config(filename=config_file)
    kconf.write_autoconf(outfile, "#pragma once\n")


def olddefconfig(config_file):
    kconf = kconfiglib.Kconfig("Kconfig")
    kconf.load_config(filename=config_file)
    kconf.write_config(filename=config_file)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--menuconfig", action="store_true")
    parser.add_argument("--defconfig", action="store_true")
    parser.add_argument("--olddefconfig", action="store_true")
    parser.add_argument("--genconfig", metavar="OUTFILE")
    parser.add_argument("--config-file", default=".config")
    args = parser.parse_args()

    if args.defconfig:
        defconfig(args.config_file)
    if args.olddefconfig:
        olddefconfig(args.config_file)
    if args.menuconfig:
        sys.argv = sys.argv[:1]
        menuconfig._main()
    if args.genconfig:
        genconfig(args.config_file, args.genconfig)


if __name__ == "__main__":
    main()
