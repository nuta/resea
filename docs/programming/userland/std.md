# Resea Standard Library
In Resea, you can't use `std` library: instead you have to use Resea's own
standard library (`resea`). It provides `libcore` and `liballoc` features
along with Resea-specific features:

- Fundamental features, e.g., `resea::{cell, mem, rc, marker, ...}`.
- Collections, e.g., `resea::vec, resea::collections{HashMap, VecDeque, ...}`.
- Channel: `resea::channel`.
- IPC stubs (described later): `resea::idl`.
- Printing macros: `trace!`, `info!`, `warn!`, `error!`, `oops!`.
- Mainloop macro: `mainloop!`.

Read [Resea Standard Library Refernece](../../userland/resea) for details.

## Using prelude
It is highly recommended to use its prelude to load frequently used primitives:
```rust
use resea::prelude::*;
```

**Note that this prelude overwrites the definition of `Result`; that is, it includes:**
```rust
pub type Result<T> = Result<T, resea::result::Error>;
```

If you'd like to use original version of `Result` (`Result<T, E>`), use `core::result::Result`.

## References
- [Resea Standard Library Refernece](../../userland/resea)
