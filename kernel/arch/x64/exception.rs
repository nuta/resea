const EXP_PAGE_FAULT: u8 = 14;

#[no_mangle]
pub extern "C" fn x64_handle_exception(exc: u8, error: u64, rip: u64) {
    printk!("Exception #%d (RIP=%p)", exc, rip);
    match exc {
        EXP_PAGE_FAULT => {
            super::paging::handle_page_fault(error);
        }
        _ => {
            panic!("Unhandled exception #{}", exc);
        }
    }
}
