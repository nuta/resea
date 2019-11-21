use crate::arch::stackframe::StackFrame;

const SYMBOL_TABLE_MAGIC: u32 = 0x2b01_2b01;
const BACKTRACE_MAX: u32 = 16;

#[repr(C, packed)]
struct SymbolEntry {
    addr: u64,
    offset: u32,
    name_len: u32,
}

#[repr(C, packed)]
struct SymbolTable {
    magic: u32,
    num_symbols: u32,
    symbols: *const SymbolEntry,
    strbuf: *const u8,
    strbuf_len: u32,
}

/// The symbol table. tools/link.py embeds it during the build.
extern "C" {
    static __symtable: SymbolTable;
}

pub struct Symbol {
    name: &'static str,
    addr: usize,
}

/// Resolves a symbol name and the offset from the beginning of symbol.
pub fn find_symbol(addr: usize) -> Option<Symbol> {
    debug_assert_eq!(unsafe { __symtable.magic }, SYMBOL_TABLE_MAGIC);

    let num_symbols = unsafe { __symtable.num_symbols };
    let symbols = unsafe {
        core::slice::from_raw_parts(__symtable.symbols, num_symbols as usize)
    };
    let strbuf = unsafe {
        core::str::from_utf8_unchecked(core::slice::from_raw_parts(
            __symtable.strbuf, __symtable.strbuf_len as usize))
    };

    // Do a binary search. Since `num_symbols` is unsigned 16-bit integer, we
    // need larger signed integer to hold -1 here, namely, i32.
    let mut l: i32 = -1;
    let mut r: i32 = num_symbols as i32;
    while r - l > 1 {
        let mid = (l + r) / 2;
        if addr >= symbols[mid as usize].addr as usize {
            l = mid;
        } else {
            r = mid;
        }
    }

    if l == -1 {
        None
    } else {
        let symbol = &symbols[l as usize];
        let str_start = symbol.offset as usize;
        let str_end = symbol.offset as usize + symbol.name_len as usize;
        let name = &strbuf[str_start..str_end];
        Some(Symbol { name, addr: symbol.addr as usize })
    }
}

/// Prints the stack trace.
pub fn backtrace() {
    info!("Backtrace:");
    let mut frame = StackFrame::current();
    for i in 0..BACKTRACE_MAX {
        let symbol = match find_symbol(frame.return_addr()) {
            Some(symbol) => symbol,
            None => break
        };

        let offset = frame.return_addr() - symbol.addr;
        info!("    #{}: {:p} {}()+0x{:x}",
              i, frame.return_addr() as *const u8, symbol.name, offset);

        frame = match frame.prev() {
            Some(frame) => frame,
            None => break,
        };
    }
}
