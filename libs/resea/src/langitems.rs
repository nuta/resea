#[lang = "eh_personality"]
extern "C" fn rust_eh_personality() {}

#[lang = "oom"]
extern "C" fn rust_oom(_layout: core::alloc::Layout) -> ! {
    loop {}
}

#[lang = "start"]
fn lang_start<T>(main: fn() -> T, _argc: isize, _argv: *const *const u8) -> isize {
    main();
    0
}

#[panic_handler]
#[no_mangle]
#[cfg(not(test))]
pub fn panic(info: &core::panic::PanicInfo) -> ! {
    internal_println!("{}: {}", env!("PROGRAM_NAME"), info);
    loop {}
}
