#pragma once
#include <types.h>

#define STACK_CANARY_VALUE 0xdeadca71

void stack_check(void);
void stack_set_canary(void);
