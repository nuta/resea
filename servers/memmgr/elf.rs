use resea::result::Error;
use resea::std::slice;
use resea::std::mem::size_of;

#[repr(C, packed)]
struct ELF64Ehdr {
    e_ident: [u8; 16],
    e_type: u16,
    e_machine: u16,
    e_version: u32,
    e_entry: u64,
    e_phoff: u64,
    e_shoff: u64,
    e_flags: u32,
    e_ehsize: u16,
    e_phentsize: u16,
    e_phnum: u16,
    e_shentsize: u16,
    e_shnum: u16,
    e_shstrndx: u16,
}

#[repr(C, packed)]
pub struct ELF64Phdr {
    pub p_type: u32,
    pub p_flags: u32,
    pub p_offset: u64,
    pub p_vaddr: u64,
    pub p_paddr: u64,
    pub p_filesz: u64,
    pub p_memsz: u64,
    pub p_align: u64,
}

pub struct ELF<'a> {
    pub entry: usize,
    pub phdrs: &'a [ELF64Phdr],
}

impl<'a> ELF<'a> {
    pub fn parse(image: &'a [u8]) -> Result<ELF<'a>, Error> {
        if image.len() < size_of::<ELF64Ehdr>() {
            return Err(Error::TooShort);
        }

        // Verify the magic.
        let ehdr = unsafe { &*(image.as_ptr() as *const ELF64Ehdr) };
        if &ehdr.e_ident[0..4] != b"\x7fELF" {
            return Err(Error::InvalidData);
        }

        let phdrs = unsafe {
            let ptr =
                image.as_ptr().add(ehdr.e_ehsize as usize) as *const ELF64Phdr;
            slice::from_raw_parts(ptr, ehdr.e_phnum as usize)
        };

        Ok(ELF { entry: ehdr.e_entry as usize, phdrs })
    }
}
