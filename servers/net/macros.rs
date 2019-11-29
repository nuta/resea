macro_rules! unwrap_or_return {
    ($option:expr, $else_value:expr) => {
        match $option {
            Some(value) => value,
            None => return $else_value,
        }
    };
}
