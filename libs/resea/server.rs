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
            let mut m = server.ch.recv().expect("failed to receive");
            let mut reply_to = Channel::from_cid(m.from);
            let has_reply = match m.header.interface_id() {
                $( $crate::idl::$interface::INTERFACE_ID =>
                    $crate::idl::$interface::Server::__handle(server, &mut m), )*
                _ => {
                    warn!("unknown message");
                    m.header =
                        $crate::message::MessageHeader::from_error(
                            $crate::error::Error::UnknownMessage);
                    true
                }
            };

            if has_reply {
                reply_to.send(&m).ok();
            }
        }
    }};
}
