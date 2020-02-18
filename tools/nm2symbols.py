#!/usr/bin/env python3
"""
    Equivalent to awk '{ print $$1, $$3 }'.
"""
import sys

for line in sys.stdin.readlines():
    cols = line.split(" ", 2)
    print(cols[0], cols[2], end="")
