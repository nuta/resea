use crate::capi::*;

pub trait ClientBase {
    fn server_task(&self) -> task_t {
        todo!()
    }
}

pub trait IntoPayload<T> {
    fn into_payload(self) -> T;
}

pub trait FromPayload<T> {
    fn from_payload(value: T) -> Self;
}

impl<T, U> IntoPayload<U> for T
where
    U: FromPayload<T>,
{
    fn into_payload(self) -> U {
        U::from_payload(self)
    }
}

impl FromPayload<size_t> for usize {
    fn from_payload(value: size_t) -> usize {
        value as usize
    }
}

impl IntoPayload<size_t> for usize {
    fn into_payload(self) -> size_t {
        self as size_t
    }
}

impl FromPayload<offset_t> for isize {
    fn from_payload(value: offset_t) -> isize {
        value as isize
    }
}

impl IntoPayload<offset_t> for isize {
    fn into_payload(self) -> offset_t {
        self as offset_t
    }
}

impl FromPayload<c_bool> for bool {
    fn from_payload(value: c_bool) -> bool {
        value != 0
    }
}

impl IntoPayload<c_bool> for bool {
    fn into_payload(self) -> c_bool {
        if self {
            1
        } else {
            0
        }
    }
}

impl IntoPayload<handle_t> for &Handle {
    fn into_payload(self) -> handle_t {
        unsafe { self.as_raw() }
    }
}

impl FromPayload<handle_t> for Handle {
    fn from_payload(value: handle_t) -> Handle {
        unsafe { Handle::from_raw(value) }
    }
}

impl IntoPayload<RawOoLString> for &str {
    fn into_payload(self) -> RawOoLString {
        unsafe { RawOoLString::from_str(self) }
    }
}

impl FromPayload<RawOoLString> for OoLString {
    fn from_payload(value: RawOoLString) -> OoLString {
        unsafe { OoLString::from_raw(value) }
    }
}

impl IntoPayload<RawOoLBytes> for &[u8] {
    fn into_payload(self) -> RawOoLBytes {
        unsafe { RawOoLBytes::from_slice(self) }
    }
}

impl FromPayload<RawOoLBytes> for OoLBytes {
    fn from_payload(value: RawOoLBytes) -> OoLBytes {
        unsafe { OoLBytes::from_raw(value) }
    }
}
