# Hiardware-Assisted Hypervisor (hv)

## Emulated Peripherals and Devices
- Serial Port
- PCI
  - virtio-blk (legacy)
- PIC (Intel 8259)
- PIT (Intel 8254)
  - *incomplete: injects IRQ #0 every 1ms regardless your settings*
- APIC
  - *incomplete: ignores all writes and does not support APIIC local timer*

## Supported Kernel Image Formats
- ELF
- ELF with [Xen PVH](https://github.com/xen-project/xen/blob/master/docs/misc/pvh.pandoc)

## Tested Guest OS
- Resea
- Linux ([config](https://gist.github.com/nuta/e76ca295ebeec02b88121a1ae7c73b9e))

## Running a hv
1. Use Linux with Nested VMX enabled.
2. Enable `CONFIG_HYPERVISOR`, `HV_SERVER`, `RAMDISK_SERVER`.
3. Run with `VMX=y` option:

```
make run VMX=y HV_GUEST_IMAGE=vmlinux.elf RAM_DISK_IMG=rootfs.ext4.img
```


## Debugging Tips (on Linux KVM)
### Enabling Trace Messages

```
echo 1 | sudo tee /sys/kernel/tracing/events/kvm/kvm_nested_vmexit/enable
echo 1 | sudo tee /sys/kernel/tracing/events/kvm/kvm_nested_intr_vmexit/enable
echo 1 | sudo tee /sys/kernel/tracing/events/kvm/kvm_nested_vmexit/enable
echo 1 | sudo tee /sys/kernel/tracing/events/kvm/kvm_nested_vmenter_failed/enable
echo 1 | sudo tee /sys/kernel/debug/tracing/tracing_on
```

### Reading Trace Messages
```
sudo watch tail /sys/kernel/debug/tracing/trace
```
