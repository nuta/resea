use resea::utils::c_str_to_str;
use resea::std::mem::size_of;

#[repr(C, packed)]
pub struct File {
    /// The file path (null-terminated).
    path: [u8; 48],
    /// The size of files.
    len: u32,
    /// The size of the padding space immediately after the file content.
    /// Padding will be added to align the beginning of the files with
    /// the page size.
    padding_len: u16,
    /// Rerserved for future use (currently unused).
    reserved: [u8; 10],
    /// File content (it comes right after the this header). Note that this
    /// it a zero-length array.
    content: [u8; 0],
}

impl File {
    pub fn len(&self) -> usize {
        self.len as usize
    }

    pub fn padding_len(&self) -> usize {
        self.padding_len as usize
    }

    pub fn path(&self) -> &str {
        unsafe {
            c_str_to_str(self.path.as_ptr())
        }
    }

    pub fn data(&self) -> &[u8] {
        unsafe {
            resea::std::slice::from_raw_parts(
                self.content.as_ptr(), self.len as usize)
        }
    }
}

const INITFS_VERSION: u32 = 1;

#[repr(C, packed)]
pub struct Initfs {
    /// The entry point of the memmgr.
    jump_code: [u8; 16],
    /// THe initfs format version. Must be `1'.
    version: u32,
    /// The size of the file system excluding this header.
    total_size: u32,
    /// The number of files in the file system.
    num_files: u32,
    /// Rerserved for future use (currently unused).
    reserved: [u8; 100],
    /// Files.
    files: [File; 0],
}

impl Initfs {
    pub fn open_dir(&self) -> Dir {
        unsafe { assert_eq!(self.version, INITFS_VERSION) };

        Dir {
            index: 0,
            num_files: self.num_files,
            next: self.files.as_ptr() as *const File,
        }
    }
}

pub struct Dir {
    index: u32,
    num_files: u32,
    next: *const File,
}

impl Dir {
    pub fn readdir(&mut self) -> Option<&'static File> {
        if self.index < self.num_files {
            let file;
            unsafe {
                file = &*self.next;
                self.index += 1;
                if self.index < self.num_files {
                    let next_addr = self.next as usize + size_of::<File>()
                        + file.len() + file.padding_len();
                    self.next = next_addr as *const File;
                }
            }
            Some(file)
        } else {
            None
        }
    }
}
