#![no_std]

use resea::{info, vec::Vec, collections::BTreeMap};

#[no_mangle]
pub fn main() {
    info!("Hello World from Rust!");
    let mut v = Vec::new();
    v.push(7);
    v.push(8);
    v.push(9);
    info!("vec test: {:?}", v);

    let mut m = BTreeMap::new();
    m.insert("a", "it works");
    info!("btreemap test: {:?}", m.get("a"));
}
