pub fn strlen(mut c_str: *const u8) -> usize {
    unsafe {
        let mut len = 0;
        while *c_str != 0 {
            len += 1;
            c_str = c_str.add(1);
        }
        len
    }
}

pub fn c_str_to_str<'a>(c_str: *const u8) -> &'a str {
    unsafe {
        let slice = core::slice::from_raw_parts(c_str, strlen(c_str));
        core::str::from_utf8_unchecked(slice)
    }
}
