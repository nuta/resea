# virtio-net
A network device driver for virtio-net version 1.x (so-called *modern device*) and 0.x (*legacy device*).

# How to Activate on QEMU
```
$ make run VIRTIO_NET=1
$ make run VIRTIO_NET=1 VIRTIO_MODERN=1
```

## References
- [Virtual I/O Device (VIRTIO) Version 1.1](http://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.html)

## Source Location
[servers/drivers/net/virtio_net](https://github.com/nuta/resea/tree/master/servers/drivers/net/virtio_net)
