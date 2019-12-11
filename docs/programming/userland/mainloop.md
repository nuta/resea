# Mainloop
Servers are implemented in the single-threaded and event-based approach. It simplies
the implementation and makes it easy to debug the program.

The `mainloop!` macro does all what you have to do: receives a message, dispatches
appropriate interface implementation, and sends a reply message if necessary.

## Example
We've defined the `rand` interface in the previous page. Let's implement `rand`
interface!

```rust
//! A pseudo-random number generator (Linear Congruential Generator). Please
//! note that this server is NOT cryptographically-secure!
use resea::prelude::*;
use resea::idl;

struct Server {
    ch: Channel,
    entropy_pool: u32,
}

impl Server {
    pub fn new() -> Server {
        // 4 is chosen by fair dice roll. Guaranteed to be random.
        // https://xkcd.com/221/
        Server {
            ch: Channel::create().unwrap(),
            entropy_pool: 4,
        }
    }
}

// Implementation for `rand` interface.
impl idl::rand::Server for Server {
    fn get_random_value(&mut self, _from: &Channel) -> Result<u32> {
        let random_value = self.entropy_pool;
        self.entropy_pool =
            self.entropy_pool.wrapping_mul(48271) % 2147483647;
        Ok(random_value)
    }

    fn add_randomness(&mut self, _from: &Channel, randomness: u32) {
        self.entropy_pool ^= randomness;
        // Returns nothing because it is a `one-way` message.
    }
}

// You should implement this trait as well.
impl resea::mainloop::Mainloop for Server {}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();
    info!("ready");

    // This macro is a event loop: receives a message, dispatches appropriate
    // interface implementation, and sends a reply message. it will never return.
    mainloop!(&mut server, [rand /* implemented interfaces */]);
}
```
