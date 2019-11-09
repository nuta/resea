use resea::result::Error;
use resea::channel::Channel;
use resea::idl::kernel::Client;
use resea::std::slice;

const STATUS_DRQ: u8 = 0x08;
const STATUS_BSY: u8 = 0x80;
const STATUS_RDY: u8 = 0x40;
const COMMAND_READ: u8 = 0x20;

pub struct IdeDevice {
    io_server: &'static Channel,
}

impl IdeDevice {
    pub fn new(io_server: &'static Channel) -> IdeDevice {
        IdeDevice {
            io_server,
        }
    }

    pub fn read_sectors(&self, sector: usize, num_sectors: usize, buf: &mut [u8]) -> Result<(), Error> {
        // Send a read command to the device.
        self.wait_device();
        self.select_num_sectors(num_sectors)?;
        self.select_sector(sector)?;
        self.select_command(COMMAND_READ)?;
        self.wait_device_for_pio();

        // Read the data.
        let read_len = num_sectors * self.sector_size();
        let buf_32: &mut [u32] = unsafe {
            slice::from_raw_parts_mut(buf.as_mut_ptr() as *mut u32, buf.len() / 4)
        };
        for i in 0..(read_len / 4) {
            buf_32[i] = self.io_server.read_ioport(0x1f0, 4)? as u32;
        }

        Ok(())
    }

    pub fn sector_size(&self) -> usize {
        512
    }

    fn drive_no(&self) -> u8 {
        0
    }

    fn select_num_sectors(&self, num_sectors: usize) -> Result<(), Error> {
        assert!(num_sectors <= 0xff);
        self.io_server.write_ioport(0x1f2, 1, num_sectors as u64)?;
        Ok(())
    }

    fn select_sector(&self, sector: usize) -> Result<(), Error> {
        let sector_low = sector & 0xff;
        let sector_mid = (sector >> 8) & 0xff;
        let sector_high = (sector >> 16) & 0xff;
        let sector_highhigh = (sector >> 24) & 0xf;

        self.io_server.write_ioport(0x1f3, 1, sector_low as u64)?;
        self.io_server.write_ioport(0x1f4, 1, sector_mid as u64)?;
        self.io_server.write_ioport(0x1f5, 1, sector_high as u64)?;
        let drive_reg =
            0xe0 /* Use LBA */ | (self.drive_no() << 4) | sector_highhigh as u8;
        self.io_server.write_ioport(0x1f6, 1, drive_reg as u64)?;
        Ok(())
    }

    fn select_command(&self, command: u8) -> Result<(), Error> {
        self.io_server.write_ioport(0x1f7, 1, command as u64)
    }

    fn read_status(&self) -> Result<u8, Error> {
        self.io_server.read_ioport(0x1f7, 1).map(|data| data as u8)
    }

    // Waits until the device gets ready.
    fn wait_device(&self) {
        loop {
            let status = self.read_status().unwrap();
            if status & STATUS_BSY == 0 {
                break;
            }
        }
    }

    // Waits until the device gets ready for a PIO operation.
    fn wait_device_for_pio(&self) {
        self.wait_device();
        while self.read_status().unwrap() & STATUS_DRQ == 0 {
            // Wait the device to read the sectors...
        }
    }
}
