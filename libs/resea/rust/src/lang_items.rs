#[panic_handler]
#[no_mangle]
#[cfg(not(test))]
pub fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {} // FIXME:
}

#[lang = "eh_personality"]
#[no_mangle]
#[cfg(not(test))]
pub fn eh_personality() {
    loop {} // FIXME:
}
