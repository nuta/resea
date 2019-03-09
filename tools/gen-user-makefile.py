#!/usr/bin/env python3
import argparse
import jinja2
import colorama
from colorama import Fore, Style

TEMPLATE = """\
target/{{ name }}/{{ target }}/$(BUILD)/{{ name }}: \\
        {{ target_path }}/start.o \\
        Makefile \\
        libs/resea/src/idl/mod.rs
	$(PROGRESS) "XARGO" {{ path }}
	cd {{ path }} && \\
		CARGO_TARGET_DIR="$(repo_dir)/target/{{ name }}" \\
		RUST_TARGET_PATH="$(repo_dir)/{{ target_path }}" \\
		PROGRAM_NAME={{ name }} \\
		RUSTFLAGS="$(RUSTFLAGS)" xargo build $(XARGOFLAGS) --target {{ target }}
	# Make paths relative.
	./tools/make-deps-relative.py "$(repo_dir)" "target/{{ name }}/{{ target }}/$(BUILD)/{{ name }}.d"
	# Remove debug info.
	$(BINUTILS_PREFIX)strip $@

initfs_files += initfs/{{ initfs_dir }}/{{ name }}.elf
initfs/{{ initfs_dir }}/{{ name }}.elf: target/{{ name }}/{{ target }}/$(BUILD)/{{ name }}
	mkdir -p $(@D)
	cp $< $@

-include target/{{ name }}/{{ target }}/$(BUILD)/{{ name }}.d
"""

def main():
    parser = argparse.ArgumentParser(description="Resea Interface stub generator.")
    parser.add_argument("--name", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--path", required=True)
    parser.add_argument("--arch", required=True)
    parser.add_argument("--target-path", required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--add-startup", action="store_true")
    args = parser.parse_args()

    vs = vars(args)
    vs.update(dict(
        initfs_dir="startups" if args.add_startup else "servers"
    ))

    mk = jinja2.Template(TEMPLATE).render(**vs)
    with open(args.output, "w") as f:
        f.write(mk)

if __name__ == "__main__":
    colorama.init(autoreset=True)
    main()
