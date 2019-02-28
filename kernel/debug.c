#include <debug.h>
#include <ipc.h>
#include <printk.h>

extern char __symbols[];
__asm__(
    ".align 8                              \n"
    ".global __symbols                     \n"
    "__symbols:                            \n"
    "   .ascii \"__SYMBOL_TABLE_START__\"  \n"
    "   .space 0x4000                      \n"
    "   .ascii \"__SYMBOL_TABLE_END__\"    \n"
);

struct symbol_table symbol_table;

static const char *find_symbol(vaddr_t vaddr, size_t *offset) {
    for (size_t i = 0; i < symbol_table.num_symbols; i++) {
        if (vaddr < symbol_table.symbols[i].addr) {
            if (i == 0) {
                *offset = 0;
                return "(maybe null)";
            }

            uint64_t strbuf_offset = symbol_table.symbols[i - 1].strbuf_offset;
            *offset = vaddr - symbol_table.symbols[i - 1].addr;
            return &symbol_table.strbuf[strbuf_offset];
        }
    }

    *offset = 0;
    return "(unknown)";
}

void backtrace(void) {
    uint64_t current_rbp;
    __asm__ __volatile__("mov %%rbp, %%rax" : "=a"(current_rbp));
    uint64_t *rbp = (uint64_t *) current_rbp;

    for (int i = 0; i < BACKTRACE_MAX; i++) {
        uint64_t return_addr = rbp[1];
        if (return_addr < KERNEL_BASE_ADDR) {
            break;
        }

        size_t offset;
        const char *name = find_symbol(return_addr, &offset);
        INFO("    #%d: %p %s()+%x", i, return_addr, name, offset);

        rbp = (uint64_t *) *rbp;
        if ((vaddr_t) rbp < KERNEL_BASE_ADDR) {
            break;
        }
    }
}

const char *find_msg_name(payload_t msg_id) {
    size_t offset;
    return find_symbol(MSG_ID_SYMBOL_PREFIX | msg_id, &offset);
}

void symbol_table_init(void) {
    struct symbol_table_header *header = (struct symbol_table_header *) &__symbols;
    if (header->magic != SYMBOL_TABLE_MAGIC) {
        PANIC("invald symbol table magic");
    }

    vaddr_t symbols_base = (vaddr_t) &__symbols + sizeof(struct symbol_table_header);
    vaddr_t strbuf_base = symbols_base + header->strbuf_offset;
    symbol_table.symbols = (struct symbol *) symbols_base;
    symbol_table.num_symbols = header->num_symbols;
    symbol_table.strbuf = (char *) strbuf_base;
}

static inline vaddr_t get_current_stack_canary_address(void) {
    return ALIGN_DOWN(arch_get_stack_pointer(), PAGE_SIZE);
}

/// Writes a kernel stack protection marker. The value `STACK_CANARY` is verified
/// by calling check_stack_canary().
///
/// FIXME: `write' is not an appropriate verb. Canary is a bird!
void write_stack_canary(vaddr_t canary) {
    *((uint32_t *) canary) = STACK_CANARY;
}

/// Verifies that stack canary is alive. If not so, register a complaint.
void check_stack_canary(void) {
    uint32_t *stack_bottom = (uint32_t *) get_current_stack_canary_address();
    if (*stack_bottom != STACK_CANARY) {
        PANIC("The stack canary is no more! This is an ex-canary!");
    }
}

/// Each CPU have to call this function once during the boot.
void init_boot_stack_canary(void) {
    write_stack_canary(get_current_stack_canary_address());
}

void debug_init(void) {
    symbol_table_init();
}
