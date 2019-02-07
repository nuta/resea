use alloc::vec::Vec;

const EI_NIDENT: usize = 16;

#[repr(C, packed)]
pub struct Header {
    pub e_ident: [u8; EI_NIDENT],
	pub e_type: u16,
	pub e_machine: u16,
	pub e_version: u32,
	pub e_entry: u64,
	pub e_phoff: u64,
	pub e_shoff: u64,
	pub e_flags: u32,
	pub e_ehsize: u16,
	pub e_phentsize: u16,
	pub e_phnum: u16,
	pub e_shentsize: u16,
	pub e_shnum: u16,
	pub e_shstrndx: u16,
}

#[derive(Debug, Clone, Copy)]
#[repr(C, packed)]
pub struct ProgramHeader {
	pub p_type: u32,
	pub p_flags: u32,
	pub p_offset: u64,
	pub p_vaddr: u64,
	pub p_paddr: u64,
	pub p_filesz: u64,
	pub p_memsz: u64,
	pub p_align: u64,
}

#[derive(Debug)]
pub struct Elf {
    pub entry: u64,
    pub program_headers: Vec<ProgramHeader>,
}

unsafe fn load_elf(content: &'static [u8]) -> Elf {
    let header = content.as_ptr() as *const Header;
    let mut program_headers = Vec::new();

    let mut offset = (*header).e_ehsize as usize;
    for _ in 0..(*header).e_phnum {
        let program_header = ((content.as_ptr() as usize) + offset) as *const ProgramHeader;
        program_headers.push((*program_header).clone());
        offset += (*header).e_phentsize as usize;
    }

    Elf {
        entry: (*header).e_entry,
        program_headers
    }
}

impl Elf {
    pub fn new(content: &'static [u8]) -> Elf {
        unsafe {
            load_elf(content)
        }
    }
}