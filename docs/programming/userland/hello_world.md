# Hello, World!
First, let's say hello to the world from your first Resea userland program!

In this section, we'll create a server named `rand`, which replies random numbers.

## 1. Clone the Repository
```
$ git clone https://github.com/seiyanuta/resea
$ cd resea
```

## 2. Create a Directory
Use `hello` server as template:
```
$ cp -r servers/hello servers/rand
```

## 3. Edit `Cargo.toml`
Open `servers/rand/main.rs` and update the package name:
```toml
[package]
name = "rand" # Update here!
version = "0.0.0"
authors = ["Your Name <yourname@example.com>"]  # and here!
edition = "2018"

[lib]
crate-type = ["staticlib"]
path = "lib.rs"

[dependencies]
resea = { path = "../../libs/resea" }
```

## 4. Edit `main.rs`
Open `servers/rand/main.rs` and update the message:
```rust
#[no_mangle]
pub fn main() {
    info!("Hello World from rand server!");
}
```

## 5. Add to `$(STARTUPS)`
Edit (or create if it does not exists) the local build configuration file
`.build.mk` to add our rand server to initfs.
```bash
$ echo "STARTUPS += rand" > .build.mk
```

## 6. Run on QEMU
```
$ make run
```

You'll see the hello world message in the build log:

```
...
[rand] Hello World from rand server!
[kernel] WARN: Exception #6
[kernel] WARN: RIP = 0000000001000022    CS  = 000000000000002b    RFL = 0000000000000202
[kernel] WARN: SS  = 0000000000000023    RSP = 0000000003000000    RBP = 0000000000000000
[kernel] WARN: RAX = 0000000000000000    RBX = 0000000000000000    RCX = 0000000001013ef8
[kernel] WARN: RDX = 0000000000000001    RSI = 0000000002fffd68    RDI = 0000000002fffdc8
[kernel] WARN: R8  = 0000000000000001    R9  = 0000000000000000    R10 = 0000000000000000
[kernel] WARN: R11 = 0000000000000206    R12 = 0000000000000000    R13 = 0000000000000000
[kernel] WARN: R14 = 0000000000000000    R15 = 0000000000000000    ERR = 0000000000000000
[kernel] WARN: NYI: ignoring thread_kill_current (#rand.22)
...
```

Ta-dah!
