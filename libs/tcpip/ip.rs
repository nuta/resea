use crate::device::Device;
use crate::ipv4::{Ipv4Addr, Ipv4Network};
use crate::transport::Port;
use resea::std::cell::RefCell;
use resea::std::fmt;
use resea::std::rc::Rc;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum NetworkProtocol {
    Ipv4,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum IpAddr {
    Ipv4(Ipv4Addr),
}

impl fmt::Display for IpAddr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            IpAddr::Ipv4(addr) => write!(f, "{}", addr),
        }
    }
}

pub struct Route {
    pub our_ipv4_addr: Ipv4Addr,
    pub ipv4_network: Ipv4Network,
    pub device: Rc<RefCell<dyn Device>>,
}

impl Route {
    pub fn new(
        ipv4_network: Ipv4Network,
        our_ipv4_addr: Ipv4Addr,
        device: Rc<RefCell<dyn Device>>,
    ) -> Route {
        Route {
            ipv4_network,
            our_ipv4_addr,
            device,
        }
    }
}
