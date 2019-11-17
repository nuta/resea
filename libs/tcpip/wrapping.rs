// TODO: Replace with core::num::Wrapping once it becomes stable.
#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct WrappingU32(u32);

impl WrappingU32 {
    pub const fn new(value: u32) -> WrappingU32 {
        WrappingU32(value)
    }

    pub fn as_u32(&self) -> u32 {
        self.0
    }

    pub fn add(&mut self, rhs: u32) {
        self.0 = self.0.wrapping_add(rhs);
    }

    pub fn abs_diff(&self, other: WrappingU32) -> u32 {
        if self.0 > other.0 {
            self.0 - other.0
        } else {
            other.0 - self.0
        }
    }
}

impl Into<WrappingU32> for u32 {
    fn into(self) -> WrappingU32 {
        WrappingU32::new(self)
    }
}
