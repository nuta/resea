use crate::prelude::*;

pub struct CString {
    /// A string. The last element is always 0, in other words, null-terminated.
    c_str: Vec<u8>,
}

impl CString {
    pub fn from_str(str: &str) -> CString {
        let mut c_str = Vec::with_capacity(str.len() + 1);
        c_str.extend_from_slice(str.as_bytes());
        c_str.push(0);
        CString { c_str }
    }

    pub fn as_cstr(&self) -> *const u8 {
        self.c_str.as_ptr()
    }
}
