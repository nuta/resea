#!/usr/bin/env python3
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("base_dir")
    parser.add_argument("dep_file")
    args = parser.parse_args()

    with open(args.dep_file, "r") as f:
        body = f.read()

    with open(args.dep_file, "w") as f:
        f.write(body.replace(args.base_dir + "/", ""))

if __name__ == "__main__":
    main()
