#!/usr/bin/env python3
import argparse
import subprocess
import tempfile
import shutil
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_dir", help="The output directory.")
    args = parser.parse_args()
    out_dir = Path(args.out_dir)

    if out_dir.exists():
        shutil.rmtree(out_dir)

    subprocess.run(["mdbook", "build"], check=True)
    shutil.copytree("book", out_dir)

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        subprocess.run(["doxygen", "misc/Doxyfile"], check=True)
        subprocess.run([
            "cargo", "doc", "--workspace", "--target-dir", tmp_dir / "userland"
        ], check=True)


        shutil.move(tmp_dir / "userland/doc", out_dir / "userland")
        shutil.move("build/kernel-docs/html", out_dir / "kernel")

if __name__ == "__main__":
    main()
