#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import colorama
from colorama import Fore, Style
import jinja2

def camelcase(text):
    new_text = ""
    spans = text.split("_")
    for span in spans:
        new_text += span.capitalize()
    return new_text

def generate_from_template(dest_path, template_path, jinja2_vars):
    tmpl = jinja2.Environment(loader=jinja2.FileSystemLoader(str(template_path)))
    tmpl.filters["camelcase"] = camelcase

    for root, dirs, files in os.walk(template_path):
        for file_ in files:
            template_file = (Path(root) / file_).relative_to(template_path)
            dest_file = dest_path / template_file.stem

            print(f"  {Style.BRIGHT}{Fore.MAGENTA}GEN{Style.RESET_ALL}\t{dest_file}")
            rendered = tmpl.get_template(str(template_file)).render(**jinja2_vars)

            try:
                os.makedirs(dest_file.parent)
            except FileExistsError:
                # Ignore if the directory already exists.
                pass

            with open(dest_file, "w") as f:
                f.write(rendered)

def get_template_dir_path(name):
    return Path(__file__).parent / "templates" / name

def generate_server(args):
    server_path = Path("servers") / args.name
    jinja2_vars = {
        "server_name": args.name
    }
    generate_from_template(server_path, get_template_dir_path("server"), jinja2_vars)

def main():
    colorama.init(autoreset=True)

    parser = argparse.ArgumentParser(description="Generates template files.")
    subparsers = parser.add_subparsers()

    server_parser = subparsers.add_parser("server")
    server_parser.add_argument("name")
    server_parser.set_defaults(func=generate_server)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
