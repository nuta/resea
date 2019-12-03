use crate::page::PageBase;
use crate::arch::thread_info::get_thread_info;

pub fn set_page_base(page_base: PageBase) -> PageBase {
    unsafe {
        core::mem::replace(&mut get_thread_info().page_base, page_base)
    }
}
