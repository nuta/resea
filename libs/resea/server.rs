use crate::channel::Channel;
use crate::message::{InterfaceId, Message, MessageHeader, Notification};
use crate::result::{Error, Result};

pub fn publish_server(interface_id: InterfaceId, server_ch: &Channel) -> Result<()> {
    use crate::idl::discovery;
    let discovery_server = Channel::from_cid(1);
    let ch = Channel::create().unwrap();
    ch.transfer_to(&server_ch).unwrap();
    discovery::Client::publish(&discovery_server, interface_id, ch)
}

pub fn connect_to_server(interface_id: InterfaceId) -> Result<Channel> {
    use crate::idl::discovery;
    let discovery_server = Channel::from_cid(1);
    discovery::Client::connect(&discovery_server, interface_id)
}

pub enum ServerResult<T> {
    Ok(T),
    Err(Error),
    NoReply,
}

pub enum DeferredWorkResult {
    Done,
    NeedsRetry,
}

pub trait Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        DeferredWorkResult::Done
    }
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
        use $crate::server::DeferredWorkResult;

        // The server struct must have 'ch' field.
        debug_assert!($server.ch.cid() != 0);
        let mut server = $server;

        // Assuming that @1 provides the timer interface.
        let timer_server = Channel::from_cid(1);
        let timer_client = Channel::create().unwrap();
        timer_client.transfer_to(&server.ch).unwrap();

        use $crate::idl::timer::Client;
        let timer_handle = timer_server.create(timer_client, 0, 0).unwrap();

        const DELAY_RESET: i32 = 20;
        const DELAY_MAX: i32 = 3000;
        let mut delay = DELAY_RESET;
        let mut needs_retry = false;

        loop {
            $crate::thread_info::alloc_and_set_page_base();
            let mut m = server.ch.recv().expect("failed to receive");
            let mut reply_to = Channel::from_cid(m.from);
            let notification = m.notification;

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

            match $crate::server::Server::deferred_work(server) {
                DeferredWorkResult::Done => {
                    // needs_retry = false;
                    delay = DELAY_RESET;
                }
                DeferredWorkResult::NeedsRetry if !notification.is_empty() => {
                    // Set a timer to retry the deferred work later.
                    info!("retrying later...");
                    timer_server.reset(timer_handle, delay, 0).unwrap();
                    needs_retry = true;
                    delay =  $crate::std::cmp::min(delay << 1, DELAY_MAX);
                }
                _ => {
                    // The timer has not yet been expired.
                }
            }
        }
    }};
}
