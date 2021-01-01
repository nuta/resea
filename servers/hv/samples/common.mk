.PHONY: default
default: build

MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

ifeq ($(V),)
.SILENT:
endif

ifeq ($(shell uname),Darwin)
LLVM_PREFIX ?= /usr/local/opt/llvm/bin/
endif

CC := $(LLVM_PREFIX)clang
LD := $(LLVM_PREFIX)ld.lld
QEMU := qemu-system-x86_64
PROGRESS := printf "  \\033[1;96m%8s\\033[0m  \\033[1;m%s\\033[0m\\n"

CFLAGS += -O0 -g3 -std=c11 -ffreestanding -fno-builtin -nostdlib -nostdinc
CFLAGS += -Wall -Wextra
CFLAGS += --target=x86_64 -mcmodel=large -fno-omit-frame-pointer
CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

QEMUFLAGS += -M microvm
#QEMUFLAGS += -enable-kvm -cpu host -m 512m
QEMUFLAGS += -nodefaults -no-user-config -nographic -serial mon:stdio

.PHONY: build
build: kernel.elf

.PHONY: clean
clean:
	rm -f kernel.elf  kernel.map kernel.qemu.elf $(objs)

.PHONY: run
run: kernel.qemu.elf
	$(PROGRESS) "QEMU" $<
	$(QEMU) $(QEMUFLAGS) -kernel kernel.qemu.elf

kernel.elf: $(objs) kernel.ld Makefile
	$(PROGRESS) "LD" $@
	$(LD) $(LDFLAGS) --script=kernel.ld -Map $(@:.elf=.map) -o $@ $(objs)

kernel.qemu.elf: kernel.elf
	$(PROGRESS) "GEN" $@
	cp $< $@
	$(shell git rev-parse --show-toplevel)/tools/make-bootable-on-qemu.py $@

%.o: %.c Makefile
	$(PROGRESS) "CC" $<
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S Makefile
	$(PROGRESS) "CC" $<
	$(CC) $(CFLAGS) -c -o $@ $<
