#!/usr/bin/env python3
import fileinput
import re

def main():
    lines = []
    funcs = []
    for line in fileinput.input():
        funcs += re.findall(r'__unittest_[0-9]+', line)
        lines.append(line)

    funcs = set(funcs)

    for func in funcs:
        print(f"void {func}(void);")

    print("void (*unittest_funcs[])(void) = {")
    for func in funcs:
        print(f"    {func},")
    print("    0")
    print("};")

    print("const char *unittest_titles[] = {")
    for func in funcs:
        for line in lines:
            m = re.match(f"const char .+ = (.*); void {func}\(void\)", line)
            if m:
                print(f"    {m.groups()[0]},")
    print("    0")
    print("};")

if __name__ == "__main__":
    main()
