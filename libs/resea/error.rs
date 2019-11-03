#[repr(u8)]
#[derive(Debug)]
pub enum Error {
    Ok = 0,
    AlreadyReceiving = 1,
    InvalidCId = 2,
    InvalidSyscall = 3,
    InvalidArg = 4,
    Unimplemented = 5,
    UnknownMessage = 6,
}
