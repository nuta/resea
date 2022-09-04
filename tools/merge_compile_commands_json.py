#!/usr/bin/env python3
import argparse
from glob import iglob
import os


def main():
    parser = argparse.ArgumentParser(
        description="Merge compilation databases into a single file."
    )
    parser.add_argument(
        "-o",
        metavar="OUTFILE",
        dest="out_file",
        required=True,
        help="The output file path.",
    )
    parser.add_argument(
        "build_dir",
        help="The directory containing the JSON files generated by clang -MJ.",
    )
    args = parser.parse_args()

    db = "["
    for file in iglob(os.path.join(args.build_dir, "**", "*.json"), recursive=True):
        if not file.endswith("compile_commands.json"):
            db += open(file).read()
    db = db.rstrip("\n,")
    db += "]\n"

    with open(args.out_file, "w") as f:
        f.write(db)


if __name__ == "__main__":
    main()