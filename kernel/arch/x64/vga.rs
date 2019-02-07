use super::asm;
use crate::arch::utils::PhyPtr;
use core::ptr;

const COLOR: u8 = 0x0f;
const X_MAX: usize = 80;
const Y_MAX: usize = 25;

static VGA_ADDR: PhyPtr<u8> = PhyPtr::new(0xb8000);
static mut CURRENT_X: usize = 0;
static mut CURRENT_Y: usize = 0;

unsafe fn vga_ptr_y(y: usize) -> *mut u8 {
    (VGA_ADDR.vaddr_as_usize() + (y * (X_MAX * 2))) as *mut u8
}

unsafe fn vga_ptr_yx(y: usize, x: usize) -> *mut u8 {
    (VGA_ADDR.vaddr_as_usize() + (y * (X_MAX * 2) + x * 2)) as *mut u8
}

unsafe fn move_cursor_to(y: usize, x: usize) {
    let pos = y * X_MAX + x;
    asm::outb(0x3d4, 0x0f);
    asm::outb(0x3d5, (pos & 0xff) as u8);
    asm::outb(0x3d4, 0x0e);
    asm::outb(0x3d5, ((pos >> 8) & 0xff) as u8);
}

unsafe fn newline() {
    CURRENT_X = 0;
    CURRENT_Y += 1;

    if CURRENT_Y == Y_MAX {
        for i in 1..Y_MAX {
            let prev = vga_ptr_y(i - 1);
            let current = vga_ptr_y(i);
            prev.write_bytes(0, X_MAX * 2);
            ptr::copy_nonoverlapping(current, prev, X_MAX * 2);
        }

        CURRENT_Y = Y_MAX - 1;
        let last = vga_ptr_y(CURRENT_Y);
        last.write_bytes(0, X_MAX * 2);
    }
}

pub unsafe fn clear_screen() {
    let ptr: *mut u8 = VGA_ADDR.as_mut_ptr();
    ptr.write_bytes(0, Y_MAX * X_MAX * 2);
}

pub unsafe fn putchar(ch: char) {
    if CURRENT_X == X_MAX {
        newline();
    }

    if ch == '\n' {
        newline();
    } else {
        let p = vga_ptr_yx(CURRENT_Y, CURRENT_X);
        p.write(ch as u8);
        p.add(1).write(COLOR as u8);
        CURRENT_X += 1;
    }

    move_cursor_to(CURRENT_Y, CURRENT_X);
}
