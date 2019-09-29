#!/usr/bin/env python3
import os
import platform

def main():
    #
    #  Search for toolchains.
    #
    llvm_prefix = ""
    grub_prefix = ""
    if platform.system() == "Darwin":
        grub_prefix += "/usr/local/opt/i386-elf-grub/bin/i386-elf-"
        llvm_prefix = "/usr/local/opt/llvm/bin/"

    # Use newer toolchain if possible (installed via apt.llvm.org).
    if os.path.exists("/usr/lib/llvm-9/bin"):
        llvm_prefix = "/usr/lib/llvm-9/bin/"
    elif os.path.exists("/usr/lib/llvm-8/bin"):
        llvm_prefix = "/usr/lib/llvm-8/bin/"

    print(f"""\
export DEFAULT_LLVM_PREFIX := {llvm_prefix}
export DEFAULT_GRUB_PREFIX := {grub_prefix}
""")

if __name__ == "__main__":
    main()
