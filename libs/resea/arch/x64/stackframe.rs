#[repr(C, packed)]
struct Frame {
    prev: *const Frame,
    return_addr: usize,
}

pub struct StackFrame(*const Frame);

impl StackFrame {
    #[inline(always)]
    pub fn current() -> StackFrame {
        let rbp: u64;
        unsafe { asm!("" : "={rbp}"(rbp)) };
        StackFrame(rbp as *const Frame)
    }

    pub fn return_addr(&self) -> usize {
        unsafe { (*self.0).return_addr }
    }

    pub fn prev(&self) -> Option<StackFrame> {
        unsafe {
            let prev_frame = (*self.0).prev;
            if prev_frame as usize == 0 {
                None
            } else {
                Some(StackFrame(prev_frame))
            }
        }
    }
}
