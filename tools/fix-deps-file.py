import argparse

parser = argparse.ArgumentParser()
parser.add_argument("deps_file")
parser.add_argument("correct_output_path")
args = parser.parse_args()

_, deps = open(args.deps_file).read().split(":", 2)
with open(args.deps_file, "w") as f:
    f.write(f"{args.correct_output_path}: {deps}")
