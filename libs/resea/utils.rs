pub unsafe fn strlen(mut c_str: *const u8) -> usize {
    let mut len = 0;
    while *c_str != 0 {
        len += 1;
        c_str = c_str.add(1);
    }
    len
}

pub unsafe fn c_str_to_str<'a>(c_str: *const u8) -> &'a str {
    let slice = core::slice::from_raw_parts(c_str, strlen(c_str));
    core::str::from_utf8_unchecked(slice)
}

pub fn align_down(value: usize, align: usize) -> usize {
    value & !(align - 1)
}

pub fn align_up(value: usize, align: usize) -> usize {
    align_down(value + (align - 1), align)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_strlen() {
        assert_eq!(unsafe { strlen(b"\0" as *const u8) }, 0);
        assert_eq!(unsafe { strlen(b"abc\0" as *const u8) }, 3);
    }

    #[test]
    fn test_cstr_to_str() {
        assert_eq!(unsafe { c_str_to_str(b"\0" as *const u8) }, "");
        assert_eq!(unsafe { c_str_to_str(b"abc\0" as *const u8) }, "abc");
    }

    #[test]
    fn test_align_down() {
        assert_eq!(align_down(0, 0x1000), 0);
        assert_eq!(align_down(0x1000, 0x1000), 0x1000);
        assert_eq!(align_down(0x1001, 0x1000), 0x1000);
        assert_eq!(align_down(0x1fff, 0x1000), 0x1000);
        assert_eq!(align_down(0x2000, 0x1000), 0x2000);
        assert_eq!(align_down(0x123456, 0x1000), 0x123000);
        assert_eq!(align_down(0x123456, 1), 0x123456);
    }

    #[test]
    fn test_align_up() {
        assert_eq!(align_up(0, 0x1000), 0);
        assert_eq!(align_up(0x1000, 0x1000), 0x1000);
        assert_eq!(align_up(0x1001, 0x1000), 0x2000);
        assert_eq!(align_up(0x1fff, 0x1000), 0x2000);
        assert_eq!(align_up(0x2000, 0x1000), 0x2000);
        assert_eq!(align_up(0x123456, 0x1000), 0x124000);
        assert_eq!(align_up(0x123456, 1), 0x123456);
    }
}
