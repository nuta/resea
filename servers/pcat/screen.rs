use resea::idl::kernel::{call_batch_write_ioport, call_get_screen_buffer};
use resea::prelude::*;

const DEFAULT_COLOR: u8 = 0x0f;
const BLANK_CHAR: u16 = 0x0f20 /* whitespace */;
const SCREEN_HEIGHT: usize = 25;
const SCREEN_WIDTH: usize = 80;

#[repr(u8)]
#[derive(Clone, Copy)]
enum Color {
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Yellow = 14,
    White = 15,
}

pub struct Screen {
    kernel_server: &'static Channel,
    screen: Page,
    cursor_x: usize,
    cursor_y: usize,
    color: Color,
}

impl Screen {
    pub fn new(kernel_server: &'static Channel) -> Screen {
        let screen = call_get_screen_buffer(kernel_server).expect("failed to get the screen page");

        Screen {
            kernel_server,
            screen,
            cursor_x: 0,
            cursor_y: 0,
            color: Color::White,
        }
    }

    pub fn clear(&mut self) {
        let screen: &mut [u16] = self.screen.as_slice_mut();
        for y in 0..SCREEN_HEIGHT {
            for x in 0..SCREEN_WIDTH {
                screen[y * SCREEN_WIDTH + x] = BLANK_CHAR;
            }
        }

        self.cursor_x = 0;
        self.cursor_y = 0;
    }

    pub fn print_str(&mut self, string: &str) {
        let mut iter = string.chars();
        while let Some(ch) = iter.next() {
            let remaining = iter.as_str();
            if ch == '\x1b' && remaining.starts_with('[') {
                // Handle a CSI sequence.
                if let Some(m_index) = remaining.find('m') {
                    let mut args = (&remaining[1..m_index]).split(';');
                    let color = match (args.next(), args.next()) {
                        (Some(_), Some(color)) => color,
                        (Some(color), None) => color,
                        (_, _) => "0",
                    };
                    
                    let mut args = (&remaining[1..m_index]).split(';');
                    //warn!("{:?} {:?}", args.next(), args.next());
                    self.color = match color {
                        "91" => Color::Red,
                        "94" => Color::Cyan,
                        "95" => Color::Magenta,
                        "33" => Color::Yellow,
                        _ => Color::White,
                    };

                    for _ in 0..(1 + m_index) {
                        iter.next();
                    }
                }

                continue;
            }

            self.draw_char(ch);
        }

        self.update_cursor();
    }

    pub fn print_char(&mut self, ch: char) {
        self.draw_char(ch);
        self.update_cursor();
    }

    fn draw_char(&mut self, ch: char) {
        if ch == '\n' || self.cursor_x >= SCREEN_WIDTH {
            self.cursor_y += 1;
            self.cursor_x = 0;
            if ch == '\n' {
                return;
            }
        }

        if (ch as u32) < 0x20 || (ch as u32) >= 0x80 {
            // Not a printable ASCII character.
            return;
        }

        if (ch as u32) == 0x7f /* backspace */ {
            if self.cursor_x > 0 {
                self.cursor_x -= 1;
                self.draw_char_at(self.cursor_y, self.cursor_x, ' ', DEFAULT_COLOR);
            }
            return;
        }

        let screen: &mut [u16] = self.screen.as_slice_mut();
        if self.cursor_y >= SCREEN_HEIGHT {
            // Scroll by one line.
            let diff = self.cursor_y - SCREEN_HEIGHT + 1;
            for from in diff..SCREEN_HEIGHT {
                let dst = (from - diff) * SCREEN_WIDTH;
                let src = from * SCREEN_WIDTH;
                screen.copy_within(src..(src + SCREEN_WIDTH), dst);
            }

            // Clear the last line.
            for i in 0..SCREEN_WIDTH {
                screen[(SCREEN_HEIGHT - diff) * SCREEN_WIDTH + i] = BLANK_CHAR;
            }

            self.cursor_y = SCREEN_HEIGHT - 1;
        }

        // Draw a character.
        self.draw_char_at(self.cursor_y, self.cursor_x, ch, self.color as u8);
        self.cursor_x += 1;
    }

    fn draw_char_at(&mut self, y: usize, x: usize, ch: char, color: u8) { 
        let screen: &mut [u16] = self.screen.as_slice_mut();
        screen[y * SCREEN_WIDTH + x] = ((color as u16) << 8) | (ch as u16);
    }

    pub fn update_cursor(&self) {
        let pos = self.cursor_y * SCREEN_WIDTH + self.cursor_x;
        call_batch_write_ioport(
            self.kernel_server,
            0x3d4,
            1,
            0x0f as u64,
            0x3d5,
            1,
            (pos & 0xff) as u64,
            0x3d4,
            1,
            0x0e as u64,
            0x3d5,
            1,
            ((pos >> 8) & 0xff) as u64,
        )
        .ok();
    }
}
