#!/usr/bin/env python3
import argparse
from pathlib import Path
import re
from html import escape
import colorama
from colorama import Fore, Style
import markdown2
import jinja2

TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Resea Book</title>
    <style>
        body {
            /* Solarized - https://ethanschoonover.com/solarized/ */
            --color-base03:  #002b36;
            --color-base02:  #073642;
            --color-base01:  #586e75;
            --color-base00:  #657b83;
            --color-base0:   #839496;
            --color-base1:   #93a1a1;
            --color-base2:   #eee8d5;
            --color-base3:   #fdf6e3;
            --color-yellow:  #b58900;
            --color-orange:  #cb4b16;
            --color-red:     #dc322f;
            --color-magenta: #d33682;
            --color-violet:  #6c71c4;
            --color-blue:    #268bd2;
            --color-cyan:    #2aa198;
            --color-green:   #859900;
            --color-highlight: #fff9a9;

            font-family: 'Lato', sans-serif;
            background: var(--color-base3);
            color: var(--color-base02);
            margin: 0;
            padding: 0;
        }

        .docs h1, h2, h3, h4, h5, h6 {
            font-family: 'Open Sans', sans-serif;
        }

        .docs h1 {
            border-bottom: var(--color-base2) 1px solid;
            padding-bottom: 3px;
        }


        .docs h2, h3 {
            margin-top: 2.5em;
        }

        nav {
            width: 250px;
            height: 100vh;
            box-sizing: border-box;
            position: fixed;
            top: 0;
            left: 0;
            padding: 10px 20px 10px;
            overflow: scroll;
            background: var(--color-base2);
        }

        nav h4 {
            margin: 25px 0 10px;
        }

        nav ul {
            margin: 0;
            padding-left: 20px;
        }

        nav li {
            line-height: 1.7em;
            list-style: none;
        }

        nav a {
            color: var(--color-base02);
            text-decoration: none;
        }

        nav a:hover {
            color: var(--color-blue);
        }

        body > main {
            max-width: 900px;
            margin-left: 330px;
            margin-top: 30px;
        }

        .docs code {
            font-family: 'Source Code Pro', monospace;
            background: var(--color-base2);
            color: var(--color-base01);
        }

        .docs table {
            background: var(--color-base3);
            border-collapse: collapse;
        }

        .docs thead {
            background: var(--color-base2);
            border-bottom: var(--color-base1) 3px solid;
        }

        .docs thead th {
            padding: 10px 0px;
        }

        .docs tr {
            border-bottom: var(--color-base2) 1px solid;
        }

        .docs td {
            padding: 10px 5px;
        }

        .src h1 {
            font-size: 17px;
            margin-top: 25px;
            margin-bottom: 10px;
        }

        .src table {
            background: var(--color-base2);
            border-collapse: collapse;
        }

        .src tr {
            border: none;
        }

        .src tr:hover {
            background: var(--color-highlight);
        }

        .src .lineno {
            font-family: 'Source Code Pro', monospace;
            font-size: 15px;
            font-weight: bold;
            text-align: right;
            padding: 0px 5px;
        }

        .src .lineno a {
            color: var(--color-base01);
            text-decoration: none;
        }

        .src .code {
            font-family: 'Source Code Pro', monospace;
            font-size: 15px;
            line-height: 1.2em;
            padding-left: 15px;
        }

        .src .code pre {
            margin: 0;
        }

        /* Override highlight.js styles. */
        .src .code pre code {
            background: transparent !important;
            padding: 0 !important;
        }

        .src .code a {
            color: var(--color-base03);
            text-decoration: none;
            border-bottom: 1px dashed var(--color-base02);
        }
    </style>
    <link href="https://fonts.googleapis.com/css?family=Lato:400,400i,700|Open+Sans:400,400i,700|Source+Code+Pro:400,700" rel="stylesheet">
    <link rel="stylesheet" type="text/css" href="https://raw.githubusercontent.com/richleland/pygments-css/master/vs.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.13.1/styles/default.min.css">
    <script src="https:////cdnjs.cloudflare.com/ajax/libs/highlight.js/9.13.1/highlight.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.13.1/languages/rust.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.13.1/languages/x86asm.min.js"></script>
</head>
<body>
<nav>
    <h4>Table of Contents</h4>
    <ul>
    {% for section in sections %}
        <li><a href="#section-{{ loop.index }}">{{ section.title }}</a></li>
    {% endfor %}
    </ul>

    <h4>Source Code</h4>
    <ul>
    {% for src in srcs %}
        <li><a href="#src-{{ loop.index }}">{{ src.path }}</a></li>
    {% endfor %}
    </ul>
</nav>
<main>
    <div class="docs">
        {% for doc in docs %}
            <section class="doc">
                <a name="section-{{ loop.index }}"></a>
                {{ doc }}
            </section>
        {% endfor %}
    </div>

    <div class="srcs">
        {% for src in srcs %}
            <section class="src">
                <header>
                    <a name="src-{{ loop.index }}"></a>
                    <h1>{{ src.path }}</h1>
                </header>
                <main>
                    <table>
                        {% for line in src.lines %}
                        <tr>
                            <td class="lineno">
                                <a href="#line-{{ line.lineno }}">{{ line.lineno }}</a>
                            </td>
                            <td class="code">
                                <a name="line-{{ line.lineno }}"></a>
                                <pre><code class="rust">{{ line.code | code_processor }}</code></pre>
                            </td>
                        </tr>
                        {% endfor %}
                    </table>
                </main>
            </section>
        {% endfor %}
    </div>
</main>

<script>
    hljs.initHighlightingOnLoad();
</script>
</body>
</html>
"""

def load_doc_dir(doc_dir):
    docs = []
    section_number = 1
    sections = []
    for md_file in Path(doc_dir).glob("**/*.md"):
        print(f"{Style.BRIGHT}{Fore.BLUE}==>{Fore.RESET} Loading {md_file}")
        body = open(md_file).read()
        html = markdown2.markdown(body, extras=["tables"])

        # Extract the title.
        title = re.search(r'<h1>(?P<title>.*?)</h1>', html).group("title")

        # Append the section number.
        html = html.replace("<h1>", f"<h1>{section_number}. ", 1)

        docs.append(html)
        sections.append({
            "title": title,
        })
        section_number += 1

    return docs, sections

TAGS = {}
def extract_tag(lineno, line):
    global TAGS
    code_part = line.split("//", 1)[0]
    m = re.search(r"struct [:whitespace:]*(?P<name>[a-zA-Z0-9_]+)", code_part)
    if m:
        TAGS[m.group("name")] = lineno

def load_src_dirs(src_dirs):
    srcs = []
    lineno = 1
    for src_dir in src_dirs:
        for src_file in Path(src_dir).glob("**/*.rs"):
            print(f"{Style.BRIGHT}{Fore.BLUE}==>{Fore.RESET} Loading {src_file}")
            lines = []
            for line in open(src_file).readlines():
                extract_tag(lineno, line)

                lines.append({
                    "lineno": lineno,
                    "code": line,
                })
                lineno += 1

            srcs.append({
                "path": src_file,
                "lines": lines,
            })

    return srcs

def code_processor(code):
    words = re.split(r'[^a-zA-Z_0-9]', code)
    html = escape(code)
    for word in set(words):
        if word in TAGS:
            html = html.replace(word, f'<a href="#line-{TAGS[word]}" class="tag">{word}</a>')
    return html

def generate(doc, src, output):
    docs, sections = load_doc_dir(doc)
    srcs = load_src_dirs(src)

    env = jinja2.Environment()
    env.filters["code_processor"] = code_processor
    body = env.from_string(TEMPLATE).render(**locals())

    with open(output, "w") as f:
        f.write(body)

def main():
    parser = argparse.ArgumentParser(description="A documentation book generator.")
    parser.add_argument("doc", help="The documentation directory.")
    parser.add_argument("src", nargs="*", help="Source directories.")
    parser.add_argument("-o", dest="output", default="index.html", help="The output file.")
    args = parser.parse_args()

    generate(args.doc, args.src, args.output)
    print(f"{Style.BRIGHT}{Fore.GREEN}==>{Fore.RESET} Successfully generated {args.output}")

if __name__ == "__main__":
    colorama.init(autoreset=True)
    main()