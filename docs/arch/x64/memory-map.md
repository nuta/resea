# Memory Map (x64)

## Physical Address Space

| Address                      | Size     | Description                              |
|------------------------------|----------|------------------------------------------|
| `0x0000_5000 - 0x0000_5fff`  | 4 KiB    | AP boot code                             |
| `0x0000_6000 - 0x0000_5fff`  | 4 KiB    | GDTR for AP boot code                    |
| `0x0010_0000 - 0x0010_1000`  | 4 KiB    | BSP boot code                            |
| `0x0010_1000 - 0x0010_0000`  | 4 KiB    | BSP boot code                            |
| `0x0010_1000 - 0x04ff_ffff`  | 78.9 MiB | Kernel image and data (including bootfs) |
| `0x0500_0000 - (end of RAM)` |          | Managed by vm server                     |

## Virtual Address Space

| Address                                         | Size         | Description                                                           |
|-------------------------------------------------|--------------|-----------------------------------------------------------------------|
| `0x0100_0000 - 0x02ff_efff`                     | 31.9 MiB     | .text, .rodata                                                        |
| `0x02ff_f000 - 0x02ff_ffff`                     | 4 KiB        | cmdline (so-called command line arguments)                            |
| `0x0300_0000 - 0x03ff_ffff`                     | 48 MiB       | .data                                                                 |
| `0x0400_0000 - 0x041f_ffff`                     | 2 MiB        | stack                                                                 |
| `0x0420_0000 - 0x0c1f_ffff`                     | 128 MiB      | heap                                                                  |
| `0x0c20_0000 - 0x0fff_ffff`                     | 62 MiB       | .bss                                                                  |
| `0x1000_0000 - 0x09ff_ffff_ffff`                | up to 9.9TiB | straight mapping (in vm server) or dynamically mapped (in the others) |
| `0xffff_8000_0000_0000 - 0xffff_8000_ffff_ffff` | 4 GiB        | Kernel (straight mapping)                                             |
