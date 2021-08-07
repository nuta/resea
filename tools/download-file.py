#!/usr/bin/env python3
import argparse
import subprocess
import tempfile
import shlex
import shutil
import glob
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_dir")
    parser.add_argument("url")
    parser.add_argument("--extract", action="store_true")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmpdir:
        outfile = Path(tmpdir) / Path(args.url).name
        subprocess.check_call(
            f"curl -fsSL {shlex.quote(args.url)} -o {outfile}",
            shell=True,
            cwd=tmpdir)

        if args.extract:
            subprocess.check_call(["tar", "xf", outfile], cwd=tmpdir)
            outfile.unlink()
            for src in glob.iglob(tmpdir + "/*"):
                shutil.move(src, args.out_dir)
        else:
            raise "not implemented"

if __name__ == "__main__":
    main()
