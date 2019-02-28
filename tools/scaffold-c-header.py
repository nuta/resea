#!/usr/bin/env python3
import argparse
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description="Generates a C header.")
    parser.add_argument("path")
    args = parser.parse_args()

    path = Path(args.path).with_suffix(".h")
    if "kernel/arch/x64" in str(path.resolve()):
        include_guard_prefix = "X64_"
    else:
        include_guard_prefix = ""

    include_guard = path.stem.upper()
    with open(path, "w") as f:
        f.write(f"""\
#ifndef __{include_guard_prefix}{include_guard}_H__
#define __{include_guard_prefix}{include_guard}_H__



#endif
""")
if __name__ == "__main__":
    main()
