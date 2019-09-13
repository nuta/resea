#include <init_args.h>
#include <resea.h>
#include <resea/math.h>
#include "alloc_pages.h"
#include "memory_map.h"

static struct zone zones[MAX_NUM_ZONES];
static int num_zones;

/// Allocates 2^order pages.
/// TODO: Implement Buddy sysytem or something sophisicated.
uintptr_t do_alloc_pages(int order) {
    size_t len = PAGE_SIZE * POW2(order);
    for (int i = 0; i < num_zones; i++) {
        if (len < zones[i].remaining_bytes) {
            uintptr_t addr = zones[i].next_alloc_addr;
            zones[i].next_alloc_addr += len;
            zones[i].remaining_bytes -= len;
            return addr;
        }
    }

    WARN("run out of memory");
    return 0;
}

void init_alloc_pages(struct memory_map *memory_maps, int num_memory_maps) {
    INFO("Memory Map:");
    int j = 0;
    for (int i = 0; i < num_memory_maps; i++) {
        struct memory_map *mmap = &memory_maps[i];
        INFO("    %p-%p: (%s, %dMiB)", mmap->start, mmap->end,
            (mmap->type == INITARGS_MEMORY_MAP_FREE) ? "free" : "reserved",
            (mmap->end - mmap->start) / 1024 / 1024);
        if (mmap->type != INITARGS_MEMORY_MAP_FREE) {
            continue;
        }

        uintptr_t start = MAX(mmap->start, FREE_MEMORY_START);
        zones[j].next_alloc_addr = start;
        zones[j].remaining_bytes = mmap->end - start;
        j++;
    }

    num_zones = j;
}
