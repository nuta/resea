use resea::cmp::min;
use resea::idl;
use resea::mem::size_of;
use resea::prelude::*;
use resea::str;

#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct FatEntry {
    name: [u8; 8],
    ext: [u8; 3],
    attr: u8,
    ntres: u8,
    time_created_ms: u8,
    time_created: u16,
    date_created: u16,
    date_last_accessed: u16,
    first_cluster_high: u16,
    time_modified: u16,
    date_modified: u16,
    first_cluster_low: u16,
    size: u32,
}

#[repr(C, packed)]
pub struct Mbr {
    jump: [u8; 3],
    oem_name: [u8; 8],
    sector_size: u16,
    sectors_per_cluster: u8,
    reserved_sectors: u16,
    number_of_fats: u8,
    root_dir_entries: u16,
    total_sectors: u16,
    media: u8,
    sectors_per_fat: u16,
    sectors_per_track: u16,
    number_of_heads: u16,
    number_of_hidden_sectors: u32,
    total_sectors32: u32,
    sectors_per_fat32: u32,
    mirroring_flags: u16,
    version: u16,
    root_dir_cluster: u32,
    fs_info_sector: u16,
    backup_sector: u16,
    boot_filename: [u8; 12],
    drive_number: u8,
    flags: u8,
    extended_boot_signature: u8,
    volume_serial_number: u32,
}

impl FatEntry {
    pub fn cluster(&self) -> Cluster {
        ((self.first_cluster_high as Cluster) << 16) | self.first_cluster_low as Cluster
    }
}

const END_OF_CLUSTER_CHAIN: u32 = 0x0fff_ffff;
type Cluster = u32;

struct Bpb {
    sectors_per_cluster: usize,
    root_dir_cluster: Cluster,
    sector_size: usize,
    fat_table_sector_start: usize,
    cluster_start_sector: usize,
}

fn parse_mbr(mbr: &[u8]) -> Bpb {
    let bpb = mbr.as_ptr() as *const Mbr;
    let sector_size = unsafe { (*bpb).sector_size as usize };
    let sectors_per_cluster = unsafe { (*bpb).sectors_per_cluster as usize };
    let fat_table_sector_start = unsafe { (*bpb).reserved_sectors as usize };
    let number_of_fats = unsafe { (*bpb).number_of_fats as usize };
    let sectors_per_fat = unsafe { (*bpb).sectors_per_fat32 as usize };
    let cluster_start_sector = fat_table_sector_start + number_of_fats * sectors_per_fat;
    let root_dir_cluster = unsafe { (*bpb).root_dir_cluster };

    trace!("FAT: root_dir_cluster = {}", root_dir_cluster);
    trace!("FAT: sectors_per_cluster = {}", sectors_per_cluster);

    Bpb {
        sectors_per_cluster,
        root_dir_cluster,
        sector_size,
        fat_table_sector_start,
        cluster_start_sector,
    }
}

pub struct FileSystem {
    storage_device: Channel,
    part_begin: usize,
    bpb: Bpb,
}

impl FileSystem {
    pub fn new(storage_device: Channel, part_begin: usize) -> Result<FileSystem> {
        let bpb = {
            let mbr = idl::storage_device::call_read(&storage_device, part_begin, 1)?;
            parse_mbr(mbr.as_bytes())
        };

        Ok(FileSystem {
            storage_device,
            bpb,
            part_begin,
        })
    }

    /// Opens a file.
    pub fn open_file(&self, path: &str) -> Option<File> {
        for frag in path.split('/') {
            // Parse the file name as: name='MAIN    ', ext='RS '
            let mut s = frag.split('.');
            let name = format!("{:<8}", s.nth(0).unwrap_or("").to_ascii_uppercase());
            let ext = format!("{:<3}", s.nth(0).unwrap_or("").to_ascii_uppercase());

            let mut current = self.bpb.root_dir_cluster;
            let mut buf = Vec::new();
            while let Some(next) = self.read_cluster(&mut buf, current) {
                let num_max_entries =
                    (self.bpb.sectors_per_cluster * self.bpb.sector_size) / size_of::<FatEntry>();
                for i in 0..num_max_entries {
                    let entries = unsafe {
                        use resea::slice::from_raw_parts;
                        from_raw_parts(buf.as_ptr() as *const FatEntry, num_max_entries)
                    };
                    let entry = entries[i];

                    if entry.name[0] == 0x00 {
                        // End of entries.
                        return None;
                    }

                    if let Ok(entry_name) = str::from_utf8(&entry.name) {
                        if let Ok(entry_ext) = str::from_utf8(&entry.ext) {
                            if entry_name == name && entry_ext == ext {
                                let file = File::new(entry.cluster(), entry.size as usize);
                                return Some(file);
                            }
                        }
                    }
                }

                current = next;
            }
        }

        None
    }

    /// Reads a cluster data and returns the next cluster.
    fn read_cluster(&self, buf: &mut Vec<u8>, cluster: Cluster) -> Option<Cluster> {
        if cluster == END_OF_CLUSTER_CHAIN {
            return None;
        }

        self.read_cluster_data(buf, cluster);
        self.read_next_cluster(cluster)
    }

    fn read_cluster_data(&self, buf: &mut Vec<u8>, cluster: Cluster) {
        let cluster_start = 2; // XXX: FAT32
        assert!(cluster as usize >= cluster_start);

        let index = cluster as usize - cluster_start;
        let sector = self.bpb.cluster_start_sector + (index * self.bpb.sectors_per_cluster);
        let data = idl::storage_device::call_read(
            &self.storage_device,
            self.part_begin + sector,
            self.bpb.sectors_per_cluster,
        )
        .unwrap();
        *buf = data.as_bytes().to_vec();
    }

    fn read_next_cluster(&self, cluster: Cluster) -> Option<Cluster> {
        let sector_offset = (size_of::<Cluster>() * cluster as usize) / self.bpb.sector_size;
        let entries_per_sector = self.bpb.sector_size / size_of::<Cluster>();
        let index = (cluster as usize) % entries_per_sector;

        let sector = self.bpb.fat_table_sector_start + sector_offset;
        let data =
            idl::storage_device::call_read(&self.storage_device, self.part_begin + sector, 1)
                .unwrap();

        let table: &[u32] = unsafe {
            use resea::slice::from_raw_parts;
            from_raw_parts(data.as_ptr() as *const u32, entries_per_sector)
        };
        let next = table[index];
        Some(next)
    }
}

pub struct File {
    first_cluster: Cluster,
    file_len: usize,
}

impl File {
    pub fn new(first_cluster: Cluster, file_len: usize) -> File {
        File {
            first_cluster,
            file_len,
        }
    }

    pub fn read(
        &self,
        fs: &FileSystem,
        buf: &mut [u8],
        offset: usize,
        len: usize,
    ) -> Result<usize> {
        debug_assert!(buf.len() >= len);
        let mut current = self.first_cluster;
        let mut data = Vec::new();
        let mut total_len = 0;
        let mut remaining = min(len, self.file_len);
        let mut off = offset;
        while let Some(next) = fs.read_cluster(&mut data, current) {
            if off < fs.bpb.sectors_per_cluster {
                let read_len = min(fs.bpb.sectors_per_cluster * fs.bpb.sector_size, remaining);
                (&mut buf[total_len..(total_len + read_len)])
                    .copy_from_slice(&data[off..(off + read_len)]);
                off = 0;
                remaining -= read_len;
                total_len += read_len;

                if remaining == 0 {
                    return Ok(total_len);
                }
            } else {
                off -= fs.bpb.sectors_per_cluster;
            }

            current = next;
        }

        unimplemented!();
    }
}
