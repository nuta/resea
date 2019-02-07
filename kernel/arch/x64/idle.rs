pub fn idle() {
    unsafe {
        asm!("sti; hlt");
    }
}
