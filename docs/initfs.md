# Initial File System
Initial File System, initfs in short, is readonly small file system is used for
launching the first user process.

## File Format
```rust
#[repr(C, packed)]
struct FileSystemHeader {
    jump_code: [u8; 16],   // e.g. `jmp start`
    version: u32,          // Must be 1.
    file_system_size: u32, // Excluding the size of this header.
    num_files: u32,        // The number of files.
    padding: [u8; 100],
}
```

```rust
#[repr(C, packed)]
struct FileHeader {
    name: [u8; 32],     // The file name.
    len: u32,           // The size of the file.
    padding_len: u32,   // The size of padding.
    reserved: [u8; 24],
}
```