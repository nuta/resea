#pragma once
#include <types.h>

#define STACK_CANARY_VALUE 0xdeadca71

void stack_check(void);
void stack_set_canary(uint32_t sp_bottom);
void stack_reset_current_canary(void);
