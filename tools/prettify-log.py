#!/usr/bin/env python3
import subprocess
import re
import sys
import os
from pathlib import Path

addr2line = os.environ.get('ADDR2LINE', 'llvm-addr2line')
elf_file = Path(os.environ.get('BUILD_DIR', 'build')) / 'resea.elf'
try:
    repo_dir = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode().strip() + '/'
except:
    repo_dir = None

def symbolize(addr, addr2line, elf_file):
    if not addr.startswith('0x'):
        addr = '0x' + addr
    source_line = subprocess.check_output([addr2line, '-e', elf_file, addr]).decode('utf-8').strip()
    if repo_dir:
        source_line = source_line.replace(repo_dir, '')
    return f"{addr.lower()}: {source_line}"

def replace_annotation(kind, arg):
    if kind == 'sym':
        return symbolize(arg, addr2line, elf_file)
    else:
        return f'(unknown annotaiton "{kind}")'

def main():
    for line in sys.stdin:
        matches = re.finditer(r'@(?P<kind>[a-z0-9]+)<(?P<arg>[^>]+)+>', line)
        for m in reversed(list(matches)):
            new_text = replace_annotation(m.group('kind'), m.group('arg'))
            line = line[:m.start()] + new_text + line[m.end():]

        for i in range(0, 5):
            try:
                print(line.rstrip())
            except BlockingIOError:
                pass
            else:
                break

if __name__ == '__main__':
    main()
