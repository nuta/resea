# Directory Strcture
In the server's directory (`servers/rand`), you'll see the following files:

- `Cargo.toml`
  - What you're already familiar with.
- `Xargo.toml`
  - Used by [xargo](https://github.com/japaric/xargo).
- `lib.rs`
  - The module.
- `main.rs`
  - The entry point. Note that servers are built as libraries: edit `lib.rs`
    if need to add something like `#![feature(...)]` instead.
