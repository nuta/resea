#!/usr/bin/env python3
import argparse
from kconfiglib import Kconfig

def main():
    parser = argparse.ArgumentParser(description="Merge .config files.")
    parser.add_argument("--outfile", required=True)
    parser.add_argument("config_files", nargs="+")
    args = parser.parse_args()

    config = Kconfig("Kconfig")
    config.warn_assign_override = False
    config.warn_assign_redun = False

    for config_file in args.config_files:
        config.load_config(config_file, replace=False)

    config.write_config(args.outfile)

if __name__ == "__main__":
    main()
