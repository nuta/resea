#pragma once
#include "arch.h"

void kernel_main(struct bootinfo *bootinfo);

extern char __boot_elf[];
