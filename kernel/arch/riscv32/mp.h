#pragma once
#include <types.h>

#define LOCKED        0x12ab
#define UNLOCKED      0xc0be
#define NO_LOCK_OWNER -1

int mp_self(void);
void mp_lock(void);
void mp_unlock(void);
