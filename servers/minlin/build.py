#!/usr/bin/env python3
from glob import glob
import os
import re
import subprocess
import shutil
import shlex
import tempfile
from pathlib import Path
from importlib import import_module
import argparse
import tarfile
import struct

BUILTIN_PACKAGES = [
    "build-essential", "curl", "sed"
]

class Package:
    def __init__(self):
        self.version = None
        self.url = None
        self.host_deps = None
        self.dockerfile = []
        self.files = {}
        self.symlinks = {}
        self.added_files = {}

    def build(self):
        pass

    def run(self, argv):
        if type(argv) == str:
            self.dockerfile.append(f"RUN {argv}")
        else:
            self.dockerfile.append(
                f"RUN {' '.join([shlex.quote(arg) for arg in argv])}")

    def env(self, key, value):
        escaped = value.replace("\"", "\\\"")
        self.dockerfile.append([f"ENV {key}={escaped}"])

    def make(self, cmd=None):
        if cmd:
            self.run(["make", f"-j{num_cpus()}", cmd])
        else:
            self.run(["make", f"-j{num_cpus()}"])

    def add_file(self, path, content):
        self.added_files[path] = content

    def set_kconfig(self, key, value):
        if type(value) == bool:
            value_str = "y" if value else "n"
        else:
            value_str = f'\\"{value}\\"'
        replace_with = f"CONFIG_{key}={value_str}"
        self.run(
            f"sh -c \"" \
                + f"sed -i -e 's/# CONFIG_{key} is not set/{replace_with}/' .config;" \
                + f"sed -i -e 's/[# ]*CONFIG_{key}=.*/{replace_with}/' .config;\"")

    def generate_dockerfile(self):
        lines = [
            "FROM ubuntu:20.04",
            f"RUN apt-get update && apt-get install -qy {' '.join(BUILTIN_PACKAGES)}"
        ]

        for path, content in self.added_files.items():
            dst_path = os.path.join("/build", path.lstrip("/"))
            tmp_path = os.path.join("add_files", path.lstrip("/"))
            Path(tmp_path).parent.mkdir(parents=True, exist_ok=True)
            if type(content) is str:
                open(tmp_path, "w").write(content)
            else:
                open(tmp_path, "wb").write(content)
            lines.append(f"ADD {tmp_path} {dst_path}")

        if self.host_deps:
            lines.append(f"RUN apt-get install -qy {' '.join(self.host_deps)}")

        if self.url:
            ext = Path(self.url).suffix
            if ext == ".tar":
                tarball = "tarball.tar"
            elif ext in [".gz", ".bz2", ".xz"]:
                tarball = f"tarball.tar{ext}"
            else:
                raise Exception(f"unknown file extension in the url: {self.url}")
            lines.append(f"RUN curl -fsSL --output {tarball} \"{self.url}\"")
            lines.append(f"RUN mkdir /build && tar xf {tarball} --strip-components=1 -C /build")
            lines.append("WORKDIR /build")

        lines += self.dockerfile
        return "\n".join(lines)

def num_cpus():
    return 16

all_symlinks = {}

def build_package(root_dir: Path, pkg):
    global all_symlinks
    root_dir = root_dir.absolute()
    cwd = os.getcwd()
    with tempfile.TemporaryDirectory() as tempdir:
        os.chdir(tempdir)
        dockerfile = pkg.generate_dockerfile()
        open("Dockerfile", "w").write(dockerfile)
        print(dockerfile)
        container_id = f"minlin-{pkg.name}-container"
        subprocess.run(
            ["docker", "build", "-t", f"minlin-{pkg.name}", "."], cwd=tempdir, check=True)
        subprocess.run(["docker", "rm", container_id])
        subprocess.run(
            ["docker", "run", "--name", container_id, "-t", f"minlin-{pkg.name}", "/bin/true"], check=True)
        for dst, src in pkg.files.items():
            dst = root_dir.joinpath(dst.lstrip("/"))
            dst.parent.mkdir(parents=True, exist_ok=True)
            if not src.startswith("/"):
                src = "/build/" + src
            subprocess.run(["docker", "cp", f"{container_id}:{src}", str(dst)], check=True)
        subprocess.run(["docker", "rm", container_id], check=True)

    all_symlinks.update(pkg.symlinks)
    os.chdir(cwd)

def get_packages():
    packages = {}
    for path in glob("packages/*"):
        name = Path(path).stem
        if name == "__pycache__":
            continue
        mod = __import__(f"packages.{name}")
        klass = getattr(getattr(mod, name), name.capitalize())
        packages[name] = klass()
    return packages

def compute_tar_checksum(header):
    checksum = 0
    for byte in header:
        checksum += byte

def main():
    parser = argparse.ArgumentParser(description="The MinLin distro build system.")
    parser.add_argument("--build-dir", help="The build directory.", required=True)
    parser.add_argument("-o", dest="outfile", required=True)
    args = parser.parse_args()

    root_dir = Path(args.build_dir)
    shutil.rmtree(root_dir, ignore_errors=True)
    os.makedirs(root_dir, exist_ok=True)

    packages = get_packages()
    for pkg in packages.values():
        pkg.build()
        build_package(root_dir, pkg)

    tar = tarfile.open(args.outfile, "w")
    tar.add(root_dir, "/", recursive=True)
    tar.close()

    # Add symlinks.
    tar_data = bytes()
    for src, dst in all_symlinks.items():
        src = src.lstrip("/")
        dst = dst.lstrip("/")
        fmt = f"100s{8+8+8}x12s12s8sc100s8s{32+32+8+8+155+12}x"
        magic  = b"ustar  \x00"
        size = b"00000000000\0"
        time_modified = b"00000000000\0"
        type_ = b"2" # symbolic link
        header = struct.pack(fmt, src.encode("ascii"), size, time_modified,
            b"", type_, dst.encode("ascii"), magic)

        from tarfile import calc_chksums
        checksum = bytes("%06o\0" % calc_chksums(header)[0], "ascii")
        header = struct.pack(fmt, src.encode("ascii"), size, time_modified,
            checksum, type_, dst.encode("ascii"), magic)
        tar_data += header

    tar_data += open(args.outfile, "rb").read()
    open(args.outfile, "wb").write(tar_data)

if __name__ == "__main__":
    main()
