use super::KERNEL_BASE_ADDR;

pub struct StackFrame {
    fp: *const u64,
}

impl StackFrame {
    #[inline(always)]
    pub fn get_current() -> StackFrame {
        StackFrame {
            fp: StackFrame::get_current_frame_pointer(),
        }
    }

    #[inline(always)]
    pub unsafe fn return_addr(&self) -> Option<usize> {
        let return_addr = self.fp.add(1).read() as usize;
        if return_addr < KERNEL_BASE_ADDR {
            // Invalid return address.
            None
        } else {
            Some(return_addr)
        }
    }

    #[inline(always)]
    pub unsafe fn move_up(&mut self) -> Result<(), ()> {
        let prev_addr = self.fp.read();
        if (prev_addr as usize) < KERNEL_BASE_ADDR {
            // Invalid frame address.
            Err(())
        } else {
            self.fp = prev_addr as *const u64;
            Ok(())
        }
    }

    #[inline(always)]
    pub fn fp(&self) -> *const u64 {
        self.fp
    }

    #[inline(always)]
    fn get_current_frame_pointer() -> *const u64 {
        unsafe {
            let rbp: u64;
            asm!("" : "={rbp}"(rbp) ::: "volatile");
            rbp as *const u64
        }
    }
}

pub fn get_stack_pointer() -> usize {
    unsafe {
        let rsp: u64;
        asm!("" : "={rsp}"(rsp));
        rsp as usize
    }
}
