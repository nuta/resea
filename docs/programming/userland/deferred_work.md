# Deferred Work
What if your server needs to call other servers? Simply calling other
servers may block. Moreover, what if the destination server does not respond
because of bug? This is why Deferred Work comes into play.

*Deferred Work* is a mechanism empolyed in `mainloop!` which calls the
`Mainloop::deferred_work` method periodically. A typical usage is to send a
message to external servers using non-blocking IPC and retry later if it would
block. This mechanism would not scale, but it works well for now.

To use deferred work, implement `deferred_work` method in your `Mainloop` trait
implementation.

## Example
```rust
use resea::prelude::*;
use resea::mainloop::{Mainloop, DeferredWorkResult};
use resea::idl::foo::{Client, nbsend_foo};

struct Server {
    ch: Channel,
    external_server: Channel,
}

impl Server {
    // ...
}

impl Mainloop for Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        match nbsend_foo(&self.external_server) {
            Err(Error::WouldBlock) | Err(Error::NeedsRetry) => {
                // `mainloop!` will invoke this deferred_work method later
                // (using timer).
                DeferredWorkResult::NeedsRetry
            }
            Err(_) => {
                panic!("failed to call foo.foo");
            }
            Ok(_) => {
            debug!("sent a request message!");
                DeferredWorkResult::Done
            }
        }
    }
}

impl Client for Server {
    pub fn foo_reply(&mut self, _from: &Channel) {
        debug!("received a reply message!");
    }
}

#[no_mangle]
fn main() {
    let mut server = /* ... */;

    // Transfer messages to `server.ch` channel in order to handle replies in
    // the mainloop.
    server.external_server.transfer_to(&server.ch).unwrap();

    mainloop!(&mut server, [], [foo /* implemented client interfaces */]);
}
```
