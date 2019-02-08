//! Debug utilities.
//!
//! `./tools/embed-symbols.py` embeds a symbol table into `__symbols`. The table
//! holds the mappings between function names and thier virtual addresses.
//!
//! To print backtraces just call `backtrace()`:
//! ```
//! fn my_handler() {
//!     // Print backtrace for debugging.
//!     crate::debug::backtrace();
//! }
//! ```
//!
use crate::arch::{StackFrame, get_stack_pointer, KERNEL_STACK_SIZE};
use crate::utils::{LazyStatic, VAddr, align_down};
use core::mem::{self, size_of};
use core::slice;
use core::str;

/// Filled by ./tools/embed-symbols.py
global_asm!(
    r#"
    .align 8
    .global __symbols
    __symbols:
       .ascii "__SYMBOL_TABLE_START__"
       .space 0x10000
       .ascii "__SYMBOL_TABLE_END__"
"#
);

static TABLE: LazyStatic<Option<SymbolTable>> = LazyStatic::new();

#[repr(C, packed)]
struct TableHeader {
    magic: [u8; 8],
    num_symbols: u64,
    strbuf_offset: u64,
    strbuf_len: u64,
}

#[repr(C, packed)]
struct SymbolAddr {
    addr: u64,
    strbuf_offset: u32,
    name_len: u32,
}

struct SymbolTable {
    addrs: &'static [SymbolAddr],
    strbuf: &'static str,
}

pub struct Symbol {
    pub name: &'static str,
    pub offset: usize,
}

impl SymbolTable {
    pub fn new(table_addr: *const u8) -> SymbolTable {
        unsafe {
            let header: *const TableHeader = mem::transmute(table_addr);
            let num_symbols = (*header).num_symbols as usize;
            let addrs_addr: *const SymbolAddr =
                mem::transmute(table_addr.add(size_of::<TableHeader>()));

            let addrs = slice::from_raw_parts(addrs_addr, num_symbols);
            let strbuf = str::from_utf8(slice::from_raw_parts(
                table_addr
                    .add(size_of::<TableHeader>())
                    .add((*header).strbuf_offset as usize),
                (*header).strbuf_len as usize,
            ))
            .expect("strbuf is not a valid utf-8 string");

            SymbolTable { addrs, strbuf }
        }
    }

    pub fn search(&self, addr: u64) -> Option<Symbol> {
        let mut prev_symbol: Option<&SymbolAddr> = None;
        // TODO: Implement binary search.
        for symbol in self.addrs {
            if addr < symbol.addr {
                if let Some(matched) = prev_symbol {
                    // Found the symbol.
                    let off = matched.strbuf_offset as usize;
                    let name = &self.strbuf[off..(off + matched.name_len as usize)];
                    let offset = (addr - matched.addr) as usize;
                    return Some(Symbol { name, offset });
                } else {
                    // Out of range.
                    return None;
                }
            }

            prev_symbol = Some(symbol);
        }

        None
    }
}

extern "C" {
    static __symbols: u8;
}

pub fn find_symbol(addr: usize) -> Option<Symbol> {
    if let Some(table) = &*TABLE {
        table.search(addr as u64)
    } else {
        None
    }
}

const BACKTRACE_MAX: usize = 16;
pub fn backtrace() {
    let mut frame = StackFrame::get_current();
    let mut remaining = BACKTRACE_MAX;
    let mut index = 1;
    while index <= BACKTRACE_MAX {
        if let Some(return_addr) = unsafe { frame.return_addr() } {
            if let Some(sym) = find_symbol(return_addr) {
                printk!("    #%d: %p %s()+%x", index, return_addr, sym.name, sym.offset);
            } else {
                printk!("    #%d: %p (unknown)", index, return_addr);
            }
        } else {
            // Invalid return address.
            break;
        }

        if unsafe { frame.move_up().is_err() } {
            break;
        }

        remaining -= 1;
        if remaining == 0 {
            break;
        }

        index += 1;
    }

    for i in 1..128 {
        let maybe_return_addr = unsafe { frame.fp().add(i).read() as usize };
        if maybe_return_addr < crate::arch::KERNEL_BASE_ADDR {
            continue;
        }

        if let Some(sym) = find_symbol(maybe_return_addr) {
            printk!("    #%d: %p ?? %s()+%x", index, maybe_return_addr, sym.name, sym.offset);
            remaining -= 1;
            if remaining == 0 {
                break;
            }
        }

        index += 1;
        if index > BACKTRACE_MAX {
            break;
        }
    }
}

const STACK_CANARY: u32 = 0xdeadca71;

/// Writes a kernel stack protection marker. The value `STACK_CANARY` is verified
/// by calling check_stack_canary().
///
/// FIXME: `write' is not an appropriate verb. Canary is a bird!
pub fn write_stack_canary(stack_bottom: VAddr) {
    unsafe {
        stack_bottom.as_ptr::<u32>().write(STACK_CANARY);
    }
}

/// Each CPU have to call this function once during the boot.
pub fn init_boot_stack_canary() {
    let stack_bottom = align_down(get_stack_pointer(), KERNEL_STACK_SIZE);
    write_stack_canary(VAddr::new(stack_bottom));
}

/// Verifies that stack canary is alive. If not so, register a complaint.
pub fn check_stack_canary() {
    let stack_bottom = align_down(get_stack_pointer(), KERNEL_STACK_SIZE);
    let value = unsafe { VAddr::new(stack_bottom).as_ptr::<u32>().read() };
    if value != STACK_CANARY {
        panic!("The stack canary is no more! This is an ex-canary!");
    }
}

pub fn init() {
    init_boot_stack_canary();

    // Verify the magic.
    let magic = unsafe {
        core::slice::from_raw_parts(&__symbols as *const u8, 8)
    };

    if magic != *b"SYMBOLS!" {
        panic!("failed to locate the symbol table");
    }

    TABLE.init(Some(SymbolTable::new(unsafe { &__symbols as *const _ })));
}
