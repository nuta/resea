use crate::result::Error;
use crate::message::{Message, MessageHeader, Notification};

pub enum ServerResult<T> {
    Ok(T),
    Err(Error),
    NoReply,
}

pub trait Server {
    fn deferred_work(&mut self) {}
    fn notification(&mut self, _notification: Notification) {}
    fn unknown_message(&mut self, m: &mut Message) -> bool {
        warn!("unknown message");
        m.header = MessageHeader::from_error(Error::UnknownMessage);
        true
    }
}

#[macro_export]
macro_rules! serve_forever {
    ($server:expr, [$($interface:ident), *]) => {{
        // The server struct must have 'ch' field.
        debug_assert!($server.ch.cid() != 0);
        let mut server = $server;
        loop {
            let mut m = server.ch.recv().expect("failed to receive");
            if unsafe { !m.notification.is_empty() } {
                $crate::server::Server::notification(server, m.notification);
            }

            let mut reply_to = Channel::from_cid(m.from);
            let has_reply = match m.header.interface_id() {
                $( $crate::idl::$interface::INTERFACE_ID =>
                    $crate::idl::$interface::Server::__handle(server, &mut m), )*
                $crate::idl::notification::INTERFACE_ID => {
                    $crate::server::Server::notification(server, m.notification);
                    false
                }
                _ => {
                    $crate::server::Server::unknown_message(server, &mut m)
                }
            };

            if has_reply {
                reply_to.send(&m).ok();
            }

            $crate::server::Server::deferred_work(server);
        }
    }};
}
