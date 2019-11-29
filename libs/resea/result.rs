#[repr(u8)]
#[derive(Debug)]
pub enum Error {
    OK = 0x00,

    // Critical errors.
    OutOfMemory   = 0x01,
    OutOfResource = 0x02,
    Unimplemented = 0x03,

    // Errors from system calls.
    InvalidCId          = 0x10,
    InvamidMsg          = 0x11,
    InvalidPayload      = 0x12,
    InvalidSyscall      = 0x13,
    InvalidPagePayload  = 0x14,
    ChannelClosed       = 0x15,
    AlreadyReceiving    = 0x16,

    // General errors.
    UnknownMessage = 0x30,
    BadRequest     = 0x31,
    NotFound       = 0x32,
    NotAcceptable  = 0x33,
    TooShort       = 0x34,
    TooLarge       = 0x35,
    AlreadyExists  = 0x36,
    ImATeapot      = 0x37,

    // Try again!
    WouldBlock = 0x50,
    NeedsRetry = 0x51,

    // Used by servers to indicate that the server does not need to send a reply
    // message.
    NoReply = 0x70,
    __NonExhaustive,
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::OK => write!(f, "OK"),
            Error::OutOfMemory => write!(f, "OutOfMemory"),
            Error::OutOfResource => write!(f, "OutOfResource"),
            Error::Unimplemented => write!(f, "Unimplemented"),
            Error::InvalidCId => write!(f, "InvalidCId"),
            Error::InvamidMsg => write!(f, "InvamidMsg"),
            Error::InvalidPayload => write!(f, "InvalidPayload"),
            Error::InvalidSyscall => write!(f, "InvalidSyscall"),
            Error::InvalidPagePayload => write!(f, "InvalidPagePayload"),
            Error::ChannelClosed => write!(f, "ChannelClosed"),
            Error::AlreadyReceiving => write!(f, "AlreadyReceiving"),
            Error::UnknownMessage => write!(f, "UnknownMessage"),
            Error::BadRequest => write!(f, "BadRequest"),
            Error::NotFound => write!(f, "NotFound"),
            Error::NotAcceptable => write!(f, "NotAcceptable"),
            Error::TooShort => write!(f, "TooShort"),
            Error::TooLarge => write!(f, "TooLarge"),
            Error::AlreadyExists => write!(f, "AlreadyExists"),
            Error::ImATeapot => write!(f, "ImATeapot"),
            Error::WouldBlock => write!(f, "WouldBlock"),
            Error::NeedsRetry => write!(f, "NeedsRetry"),
            Error::NoReply => write!(f, "NoReply"),
            Error::__NonExhaustive => write!(f, "(invalid error)"),
        }
    }
}

pub type Result<T> = core::result::Result<T, Error>;
