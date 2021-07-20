use crate::capi::error_t;

#[repr(transparent)]
pub struct Error(error_t);

impl From<error_t> for Error {
    fn from(value: error_t) -> Self {
        Error(value)
    }
}

pub type Result<T> = ::core::result::Result<T, Error>;
