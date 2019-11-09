//
//  Kernel Address Santizer (KASan) runtime.
// (https://github.com/google/kasan/wiki)
//
//  The implementation is in the very early stage: redzones, stack overflow
//  detection, shadow propagation, and other features are not yet supported.
//
//  Even though it is incomplete, it's able to detect some memory bugs: NULL
//  pointer dereference, use of an uninitialized memory, etc.
//
// TODO: SMP support: kasan_enabled should be a CPU-local variable.
//
#include <arch.h>
#include <memory.h>
#include <support/kasan.h>

static bool kasan_enabled = false;

static inline uint8_t *get_asan_shadow_memory(vaddr_t addr) {
    ASSERT(addr >= KERNEL_BASE_ADDR);
    return (uint8_t *) ASAN_SHADOW_MEMORY + into_paddr((void *) addr);
}

static bool set_kasan_enabled(bool state) {
    bool prev_state = kasan_enabled;
    kasan_enabled = state;
    return prev_state;
}

static bool needs_asan_check(vaddr_t addr) {
    return kasan_enabled
           && arch_asan_is_kernel_address(addr)
           && addr < (vaddr_t) from_paddr(STRAIGHT_MAP_ADDR);
}

/// Checks whether the memory read is valid.
static void check_load(vaddr_t addr, size_t size) {
    if (!needs_asan_check(addr)) {
        return;
    }

    // Disable ASan temporarily to prevent infinte recursive errors.
    set_kasan_enabled(false);

    if (!addr) {
        PANIC("NULL pointer dereference (read) at %p", addr);
    }

    if (addr <= KERNEL_BASE_ADDR) {
        PANIC("invalid memory read access at %p", addr);
    }

    for (size_t i = 0; i < size; i++) {
        uint8_t *shadow = get_asan_shadow_memory(addr + i);
        switch (*shadow) {
        case ASAN_VALID:
            break;
        case ASAN_FREED:
            PANIC("asan: detected a use-after-free bug of %p", addr);
        case ASAN_NOT_ALLOCATED:
            PANIC("asan: detected an use of unallocated memory at %p", addr);
        case ASAN_UNINITIALIZED:
            PANIC("asan: detected an use of uninitialized memory at %p", addr);
        default:
            PANIC("asan: bad shadow memory state for %p (shadow=%p, value=%x)",
                  addr, shadow, *shadow);
        }
    }

    set_kasan_enabled(true);
}

/// Checks whether the memory write is valid.
static void check_store(vaddr_t addr, size_t size) {
    if (!needs_asan_check(addr)) {
        return;
    }

    // Disable ASan temporarily to prevent infinte recursive errors.
    set_kasan_enabled(false);

    if (!addr) {
        PANIC("NULL pointer dereference (write) at %p", addr);
    }

    if (addr <= KERNEL_BASE_ADDR) {
        PANIC("invalid memory write access at %p", addr);
    }

    for (size_t i = 0; i < size; i++) {
        uint8_t *shadow = get_asan_shadow_memory(addr + i);
        switch (*shadow) {
        case ASAN_VALID:
            break;
        case ASAN_UNINITIALIZED:
            // TODO: Propagate shadow memory.
            *shadow = ASAN_VALID;
            break;
        case ASAN_FREED:
            PANIC("asan: detected a use-after-free bug of %p", addr);
        case ASAN_NOT_ALLOCATED:
            PANIC("asan: detected an use of unallocated memory at %p", addr);
        default:
            PANIC("asan: bad shadow memory state for %p (shadow=%p, value=%x)",
                  addr, shadow, *shadow);
        }
    }

    set_kasan_enabled(true);
}

// ASan interfaces used by the compiler.
void __asan_load1_noabort(vaddr_t addr)            { check_load(addr, 1); }
void __asan_load2_noabort(vaddr_t addr)            { check_load(addr, 2); }
void __asan_load4_noabort(vaddr_t addr)            { check_load(addr, 4); }
void __asan_load8_noabort(vaddr_t addr)            { check_load(addr, 8); }
void __asan_loadN_noabort(vaddr_t addr, size_t n)  { check_load(addr, n); }
void __asan_store1_noabort(vaddr_t addr)           { check_store(addr, 1); }
void __asan_store2_noabort(vaddr_t addr)           { check_store(addr, 2); }
void __asan_store4_noabort(vaddr_t addr)           { check_store(addr, 4); }
void __asan_store8_noabort(vaddr_t addr)           { check_store(addr, 8); }
void __asan_storeN_noabort(vaddr_t addr, size_t n) { check_store(addr, n); }
void __asan_handle_no_return(void) { /* TODO: */; }

/// Fills the shadow memory of the given memory area with the tag.
void kasan_init_area(enum kasan_shadow_tag tag, void *ptr, size_t len) {
    bool kasan_state = set_kasan_enabled(false);
    inlined_memset(get_asan_shadow_memory((vaddr_t) ptr), tag, len);
    set_kasan_enabled(kasan_state);
}

/// Detects double-free bugs.
void kasan_check_double_free(struct free_list *free_list) {
    uint8_t *shadow = get_asan_shadow_memory((vaddr_t) &free_list->padding);
    if (*shadow == ASAN_FREED) {
        PANIC("asan: detected double-free bug (free_list=%p)", free_list);
    }
}

/// Initializes the ASan runtime.
void kasan_init(void) {
    arch_asan_init();

    // Kmalloc small objects.
    kasan_init_area(ASAN_NOT_ALLOCATED, (void *) OBJECT_ARENA_ADDR,
                  OBJECT_ARENA_LEN);
    // Kmalloc page objects.
    kasan_init_area(ASAN_NOT_ALLOCATED, (void *) PAGE_ARENA_ADDR,
                  PAGE_ARENA_LEN);

    set_kasan_enabled(true);
}
