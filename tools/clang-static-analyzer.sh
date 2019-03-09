#!/bin/sh
set -eux
VERSION=279

make clean

if [[ ! -d checker-$VERSION ]]; then
    wget https://clang-analyzer.llvm.org/downloads/checker-$VERSION.tar.bz2
    tar xvf checker-$VERSION.tar.bz2
fi

./checker-$VERSION/bin/scan-build --view -stats make
