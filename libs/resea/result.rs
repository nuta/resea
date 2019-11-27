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
    TooShort = 7,
    InvalidData = 8,
    NotFound = 9,
    InvalidHandle = 10,
    NeedsRetry = 22,
    NoReply = 23,
    WouldBlock = 24,
    __NonExhaustive,
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::Ok => { write!(f, "Ok") }
            Error::AlreadyReceiving => { write!(f, "AlreadyReceiving") }
            Error::InvalidCId => { write!(f, "InvalidCId") }
            Error::InvalidSyscall => { write!(f, "InvalidSyscall") }
            Error::InvalidArg => { write!(f, "InvalidArg") }
            Error::Unimplemented => { write!(f, "Unimplemented") }
            Error::UnknownMessage => { write!(f, "UnknownMessage") }
            Error::TooShort => { write!(f, "TooShort") }
            Error::InvalidData => { write!(f, "InvalidData") }
            Error::NotFound => { write!(f, "NotFound") }
            Error::InvalidHandle => { write!(f, "InvalidHandle") }
            Error::NeedsRetry => { write!(f, "NeedsRetry") }
            Error::NoReply => { write!(f, "NoReply") }
            Error::WouldBlock => { write!(f, "WouldBlock") }
            _ => { Ok(()) }
        }
    }
}

pub type Result<T> = core::result::Result<T, Error>;
