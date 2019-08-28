#!/usr/bin/env python3
import argparse
import os
import subprocess
import tempfile
import re
from pathlib import Path

def enumerate_files():
    sources = [Path("Makefile")]
    for root, dirs, files in os.walk("."):
        if not re.match(r"^\./(kernel|apps|libs|tools)", root):
            continue
        for file in files:
            path = Path(root) / file
            if path.suffix not in [".c", ".h", ".mk", ".py", ""]:
                continue
            sources.append(path)
    return sources

def merge_files(files):
    code = ""
    for file in files:
        code += f"//\n"
        code += f"//  {file}\n"
        code += f"//\n"
        code += file.open().read() + "\n"
    return code

def main():
    parser = argparse.ArgumentParser(description="(physically) print the source code.")
    parser.add_argument("-o", metavar="OUTFILE", default="code.ps", dest="out_file",
        help="The output file (in .ps format).")
    args = parser.parse_args()
    files = enumerate_files()
    code = merge_files(files)
    with tempfile.NamedTemporaryFile("w") as f:
        f.write(code)
        f.flush()
        subprocess.check_call(["enscript", "--escapes", "--columns=2", "--highlight=c", "--color",
            "--line-numbers", "--landscape", "-o", args.out_file, f.name])

if __name__ == "__main__":
    main()
