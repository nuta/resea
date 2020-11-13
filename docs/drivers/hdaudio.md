# hdaudio (experimental)
A device driver for the Intel HD Audio controller and speaker.

Currently, this driver is an experimental implementation and it
just play a hard-coded sound data forever.

## Building
You need a wav file (44.1kHz, 16-bits, 2 channels) to play. Generate a `sound_data.h` by `wav2c.py`:
```
$ cd servers/experimental/hdaudio
$ ./wav2c.py <your_wav_file>
```

## Testing on QEMU
Enable the Intel HD audio device by `HD_AUDIO`:

```
$ make run HD_AUDIO=y
```

## References
- [ATA PIO Mode - OSDev Wiki](https://wiki.osdev.org/ATA_PIO_Mode)

## Source Location
[servers/drivers/blk/ide](https://github.com/nuta/resea/tree/master/servers/drivers/blk/ide)
