use crate::prelude::*;

pub fn publish_server(interface_id: InterfaceId, server_ch: &Channel) -> Result<()> {
    use crate::idl::discovery;
    let discovery_server = Channel::from_cid(1);
    let ch = Channel::create()?;
    ch.transfer_to(&server_ch)?;
    discovery::call_publish(&discovery_server, interface_id, ch)
}

pub fn connect_to_server(interface_id: InterfaceId) -> Result<Channel> {
    use crate::idl::discovery;
    let discovery_server = Channel::from_cid(1);
    discovery::call_connect(&discovery_server, interface_id)
}
