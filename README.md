Resea
======
Resea is a new *pure* microkernel-based operating system featuring:

- A straight-forward, flexible, and small microkernel.
- Supports x86_64.
- Written almost entirely in Rust.

```sh
make setup                # Install build prerequisites (LLVM, GNU Binutils, ...).
make build                # Build a kernel executable (debug build).
make build BUILD=release  # Build a kernel executable (release build).
make build V=1            # Build a kernel executable with verbose command output.
make run                  # Run Resea on an emulator.
make clean                # Remove generated files.
make book                 # Generate a HTML formatted book from documentation.
```

License
-------
MIT or CC0. Choose whichever you prefer.
