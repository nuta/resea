use resea::channel::Channel;
use resea::message::Page;

const DEFAULT_COLOR: u16 = 0x0f;
const SCREEN_HEIGHT: usize = 25;
const SCREEN_WIDTH: usize = 80;

pub struct Screen {
    screen: Page,
    cursor_x: usize,
    cursor_y: usize,
}

impl Screen {
    pub fn new(kernel_server: &'static Channel) -> Screen {
        use resea::idl::kernel::Client;
        let screen = kernel_server
            .get_screen_buffer()
            .expect("failed to get the screen page");

        Screen {
            screen,
            cursor_x: 0,
            cursor_y: 0,
        }
    }

    pub fn clear(&mut self) {
        let screen: &mut [u16] = self.screen.as_slice_mut();
        for y in 0..SCREEN_HEIGHT {
            for x in 0..SCREEN_WIDTH {
                screen[y * SCREEN_WIDTH + x] = 0;
            }
        }
    }

    pub fn print_str(&mut self, string: &str) {
        for ch in string.chars() {
            self.draw_char(ch);
        }

        self.update_cursor();
    }

    pub fn print_char(&mut self, ch: char) {
        self.draw_char(ch);
        self.update_cursor();
    }

    fn draw_char(&mut self, ch: char) {
        let screen: &mut [u16] = self.screen.as_slice_mut();
        if ch == '\n' || self.cursor_x >= SCREEN_WIDTH {
            self.cursor_y += 1;
            self.cursor_x = 0;
            if ch == '\n' {
                return;
            }
        }

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
                screen[(SCREEN_HEIGHT - diff) * SCREEN_WIDTH + i] = 0;
            }

            self.cursor_y = SCREEN_HEIGHT - 1;
        }

        // Draw a character.
        screen[self.cursor_y * SCREEN_WIDTH + self.cursor_x] =
            (DEFAULT_COLOR << 8) | (ch as u8) as u16;
        self.cursor_x += 1;
    }

    pub fn update_cursor(&self) {
        // TODO:
    }
}
