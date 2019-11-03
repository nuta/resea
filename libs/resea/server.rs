use crate::error::Error;
use crate::channel::Channel;
use crate::message::Message;

pub trait Server {}

#[macro_export]
macro_rules! serve_forever {
    ($server:expr, [$($interface:ident), *]) => {{
        // The server struct must have 'ch' field.
        debug_assert!($server.ch.cid() != 0);
        let mut server = $server;
        loop {
            let m = server.ch.recv().expect("failed to receive");
            let mut reply_to = Channel::from_cid(m.from);
            let reply = match m.header.interface_id() {
                $( $crate::idl::$interface::INTERFACE_ID =>
                    $crate::idl::$inteface::Server::handle(server, m), )*
                _ => {
                    // println!("unknown message");
                    Some(Err(Error::UnknownMessage))
                }
            };

            match reply {
                Some(Ok(r)) => {
                    // FIXME: Don't panic here!
                    reply_to.send(r).expect("failed to reply");
                }
                Some(Err(err)) => {
                    // FIXME: Don't panic here!
                    reply_to.send_err(err).expect("failed to reply");
                }
                None => {
                    // Don't reply: nothing to do.
                }
            }
        }
    }};
}
