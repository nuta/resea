#[macro_export]
macro_rules! serve_forever {
    ($server:expr, $server_ch:expr, [$($interface:ident), *]) => {
        loop {
            $server_ch.wait().expect("failed to wait");
            let header = $server_ch.peek_header();
            let r = match header.interface_id() {
            $(
                resea::idl::$interface::INTERFACE_ID => {
                    resea::idl::$interface::Server::handle($server, &$server_ch, header);
                }
            )*
                _ => {
                    println!("{}: unknown message: {:04x}",
                        env!("PROGRAM_NAME"), header.msg_id());
                }
            };
        }
    };
}
