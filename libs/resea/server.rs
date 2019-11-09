use crate::result::Error;
use crate::channel::Channel;
use crate::message::{Message, Notification, InterfaceId};

const NOTIFICATION_INTERFACE_ID: InterfaceId = 100;

pub trait Server {
    fn deferred_work(&mut self) {}
    fn notification(&mut self, notification: Notification) {}
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
                NOTIFICATION_INTERFACE_ID => {
                    $crate::server::Server::notification(server, m.notification);
                    false
                }
                _ => {
                    warn!("unknown message");
                    m.header =
                        $crate::message::MessageHeader::from_error(
                            $crate::result::Error::UnknownMessage);
                    true
                }
            };

            if has_reply {
                reply_to.send(&m).ok();
            }

            $crate::server::Server::deferred_work(server);
        }
    }};
}
