extern "C" {
    fn main();
}

#[no_mangle]
pub fn resea_main() {
    crate::init::init();
    unsafe {
        main();
    }
}
