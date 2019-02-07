use core::mem::size_of;
use core::slice;
use alloc::string::String;
use alloc::collections::BTreeMap;

#[derive(Clone, Copy)]
pub struct File {
    pub content: &'static [u8],
}

pub struct InitFs {
    files: BTreeMap<String, File>,
}

#[repr(C, packed)]
struct FileSystemHeader {
    jump_code: [u8; 16],   // e.g. `jmp start`
    version: u32,          // Must be 1.
    file_system_size: u32, // Excluding the size of this header.
    num_files: u32,        // The number of files.
    padding: [u8; 100],
}

#[repr(C, packed)]
struct FileHeader {
    name: [u8; 32],     // The file name.
    len: u32,           // The size of the file.
    padding_len: u32,   // The size of padding.
    reserved: [u8; 24],
}

pub fn find_null(chars: &[u8]) -> usize {
    let mut index = 0;
    for c in chars {
        if *c as char == '\0' {
            return index;
        }

        index += 1;
    }

    return chars.len();
}

pub unsafe fn load_files(addr: usize) -> BTreeMap<String, File> {
    let mut files = BTreeMap::new();
    let fs_header = addr as *const FileSystemHeader;
    let mut file_offset = addr + size_of::<FileSystemHeader>();
    for i in 0..(*fs_header).num_files {
        let header = file_offset as *const FileHeader;
        let mut name_vec = (*header).name.to_vec();
        let name_len = find_null(&name_vec);
        name_vec.truncate(name_len);
        let name = String::from_utf8(name_vec).unwrap();
        let len = (*header).len as usize;

        let content_ptr = (file_offset + size_of::<FileHeader>()) as *const u8;
        let content = slice::from_raw_parts(content_ptr, len);

        println!("file #{}: name=\"{}\", len={}, addr={:x}", i, name, len, file_offset);
        files.insert(name, File { content });
        file_offset += size_of::<FileHeader>() + len + (*header).padding_len as usize;
    }

    files
}

impl InitFs {
    pub fn new(image: *const u8) -> InitFs {
        let files = unsafe { load_files(image as usize) };
        InitFs { files }
    }

    pub fn files(&self) -> &BTreeMap<String, File> {
        &self.files
    }
}