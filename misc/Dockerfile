#
# A build environment in Docker for those who are tidy.
#
# Usage:
#
#    $ make defconfig
#    $ docker build -t resea-build -f misc/Dockerfile .
#    $ docker run -it resea-build -v build/docker:/resea/build make -j8
#    $ ls build/docker
#
FROM ubuntu:19.04

RUN    echo "deb mirror://mirrors.ubuntu.com/mirrors.txt disco main restricted universe multiverse" > /etc/apt/sources.list \
    && echo "deb mirror://mirrors.ubuntu.com/mirrors.txt disco-updates main restricted universe multiverse" >> /etc/apt/sources.list \
    && echo "deb mirror://mirrors.ubuntu.com/mirrors.txt disco-security main restricted universe multiverse" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -qy --no-install-recommends \
        llvm lld clang make git \
        python3 python3-pip python3-setuptools python3-wheel python3-dev \
        grub xorriso mtools qemu-system-x86 \
    && apt-get clean

COPY . /resea
WORKDIR /resea

RUN pip3 install -r tools/requirements.txt
