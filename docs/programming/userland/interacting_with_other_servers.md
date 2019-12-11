# Interacting with Other Servers

## Pre-Installed Channels
These channels are connected by the kernel or memmgr during process creation.

- `@1`: Connected to the kernel server.
- `@2`: Connected to memmgr (except memmgr's process itself).

## Calling Servers
To call servers use IPC call stubs (`call_*`) in `resea::idl` module:

```rust
use resea::idl;
let kernel_server = Channel::from_cid(2);

let uptime = idl::kernel::call_read_stats(&kernel_server).unwrap().0;
info!("uptime: {} seconds\n", uptime / 1000);
```

## Sending a Message
To call servers use IPC call stubs (`send_*` or `nbsend_*`) in `resea::idl` module.

`nbsend_*` functions perform non-blocking IPC: it returns `Err::WouldBlock` if
the send operation would block.

## Service Discovery
In order to connect to other servers, memmgr provides `discovery` interface,
a service discovery mechanism for userland programs.

### Connecting to a Server
To connect to a server, use `resea::server::connect_to_server`:

```rust
use resea::server::connect_to_server;

let keyboard_server = connect_to_server(resea::idl::keyboard_device::INTERFACE_ID)
    .expect("failed to connect to a keyboard_device server");
```

### Publishing a Server
To publish to a service, implement `idl::server::Server` and call
`resea::server::publish_server`:

```rust
use resea::server::publish_server;

struct Server {
    ch: Channel,
}

impl idl::server::Server for Server {
    fn connect( &mut self, _from: &Channel, interface: InterfaceId) -> Result<(InterfaceId, Channel)> {
        // Create and return a new client channel.
        let client_ch = Channel::create()?;
        client_ch.transfer_to(&self.ch)?;
        Ok((interface, client_ch))
    }
}

#[no_mangle]
pub fn main() {
    /* ... */
    publish_server(INTERFACE_ID, &server.ch).unwrap();
    /* ... */
    mainloop!(/* ... */);
}
```
