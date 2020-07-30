#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/klog.h>
#include <resea/async.h>
#include <string.h>

static task_t bootstrap_server = 1;
static task_t display_server;
static task_t kbd_server;

static int cursor_x = 0;
static int cursor_y = 0;
static enum textscreen_color text_color = TEXTSCREEN_COLOR_NORMAL;
static int width;
static int height;
static bool in_esc = false;
static int color_code = 0;

static void newline(void) {
    cursor_x = 0;
    if (cursor_y < height - 1) {
        cursor_y++;
    } else {
        struct message m;
        m.type = TEXTSCREEN_SCROLL_MSG;
        ipc_send(display_server, &m);
    }
}

static void update_cursor(void) {
    struct message m;
    m.type = TEXTSCREEN_MOVE_CURSOR_MSG;
    m.textscreen_move_cursor.y = cursor_y;
    m.textscreen_move_cursor.x = cursor_x;
    ipc_send(display_server, &m);
}

static void clear_screen(void) {
    struct message m;
    m.type = TEXTSCREEN_CLEAR_MSG;
    ipc_send(display_server, &m);
}

static void draw_char(int x, int y, char ch, enum textscreen_color fg_color, enum textscreen_color bg_color) {
    struct message m;
    m.type = TEXTSCREEN_DRAW_CHAR_MSG;
    m.textscreen_draw_char.ch = ch;
    m.textscreen_draw_char.x = x;
    m.textscreen_draw_char.y = y;
    m.textscreen_draw_char.fg_color = fg_color;
    m.textscreen_draw_char.bg_color = bg_color;
    ipc_send(display_server, &m);
}

void logputc(char ch) {
    if ((ch < 0x20 && ch != '\n' && ch != '\e') || ch >= 0x7f) {
        return;
    }

    if (ch == '\e') {
        in_esc = true;
        color_code = 0;
        return;
    }

    // Handle escape sequences like "\e[1;33m".
    if (in_esc) {
        if ('0' <= ch && ch <= '9') {
            if (color_code > 100) {
                // Invalid (or corrupted) escape sequence. Ignore it to prevent
                // a panic by UBSan.
            } else {
                color_code = (color_code * 10) + (ch - '0');
            }
        } else if (ch == ';') {
            color_code = 0;
        } else if (ch == 'm') {
            in_esc = false;
            switch (color_code) {
                case 32:
                    text_color = TEXTSCREEN_COLOR_GREEN;
                    break;
                case 33:
                    text_color = TEXTSCREEN_COLOR_YELLOW;
                    break;
                case 91:
                    text_color = TEXTSCREEN_COLOR_RED;
                    break;
                case 96:
                    text_color = TEXTSCREEN_COLOR_CYAN;
                    break;
                default:
                    text_color = TEXTSCREEN_COLOR_NORMAL;
            }
        }

        return;
    }

    if (ch == '\n') {
        newline();
    }

    if (ch != '\n') {
        draw_char(cursor_x, cursor_y, ch, text_color, TEXTSCREEN_COLOR_BLACK);
        cursor_x++;
        if (cursor_x == width) {
            newline();
        }
    }

    update_cursor();
}

void logputstr(const char *str) {
    while (*str) {
        logputc(*str);
        str++;
    }
}

static void echo_command(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        logputstr(argv[i]);
        logputc(' ');
    }

    logputc('\n');
}

static void clear_command(__unused int argc, __unused char **argv) {
    clear_screen();
    cursor_x = 0;
    cursor_y = 0;
}

static void help_command(__unused int argc, __unused char **argv) {
    logputstr("help   -  Print this message.\n");
    logputstr("echo   -  Print strings.\n");
    logputstr("clear  -  Clear the screen.\n");
}

static void log_command(__unused int argc, __unused char **argv) {
    while (true) {
        char buf[512];
        size_t read_len = klog_read(buf, sizeof(buf));
        if (!read_len) {
            break;
        }

        for (size_t i = 0; i < read_len; i++) {
            logputc(buf[i]);
        }
    }
}

struct command {
    const char *name;
    void (*run)(int argc, char **argv);
};

static struct command commands[] = {
    { .name = "echo", .run = echo_command },
    { .name = "clear", .run = clear_command },
    { .name = "log", .run = log_command },
    { .name = "help", .run = help_command },
    { .name = NULL, .run = NULL },
};

#define CMDLINE_MAX 128

void skip_whitespaces(char **cmdline) {
    while (**cmdline == ' ') {
        (*cmdline)++;
    }
}

int parse(char *cmdline, char **argv, int argv_max) {
    skip_whitespaces(&cmdline);
    if (*cmdline == '\0') {
        return 0;
    }

    int argc = 1;
    argv[0] = cmdline;
    while (*cmdline != '\0') {
        if (*cmdline == ' ') {
            *cmdline++ = '\0';
            skip_whitespaces(&cmdline);
            argv[argc] = cmdline;
            argc++;
            if (argc == argv_max - 1) {
                break;
            }
        } else {
            cmdline++;
        }
    }

    argv[argc] = NULL;
    return argc;
}

static error_t launch_task(const char *task_name) {
    struct message m;
    m.type = LAUNCH_TASK_MSG;
    m.launch_task.name = task_name;
    return ipc_call(bootstrap_server, &m);
}

void run(const char *cmd_name, int argc, char **argv) {
    for (int i = 0; commands[i].name != NULL; i++) {
        if (!strcmp(commands[i].name, cmd_name)) {
            commands[i].run(argc, argv);
            return;
        }
    }

    error_t err = launch_task(cmd_name);
    switch (err) {
        case OK:
            break;
        case ERR_NOT_FOUND:
            logputstr("unknown command or task: ");
            logputstr(cmd_name);
            logputstr("\n");
            break;
        default:
            WARN("failed launch a task '%s': %s", cmd_name, err2str(err));
    }
}

static char cmdline[CMDLINE_MAX];
static int cursor = 0;

void prompt(void) {
    logputstr(">>> ");
    cursor = 0;
}

static void input(char ch) {
    int argv_max = 8;
    char **argv = malloc(sizeof(char *) * argv_max);
    switch (ch) {
        case '\b':
            if (cursor > 0) {
                cursor_x--;
                draw_char(cursor_x, cursor_y, ' ', TEXTSCREEN_COLOR_NORMAL, TEXTSCREEN_COLOR_BLACK);
                update_cursor();
                cursor--;
                cmdline[cursor] = '\0';
            }
            break;
        default:
            logputc(ch);
            switch (ch) {
                case '\n': {
                    cmdline[cursor] = '\0';
                    int argc = parse(cmdline, argv, argv_max);
                    if (argc > 0) {
                        run(argv[0], argc, argv);
                    }
                    prompt();
                    break;
                }
                default:
                    if (cursor == CMDLINE_MAX - 1) {
                        logputstr("\nshell: too long input\n");
                        prompt();
                    } else {
                        cmdline[cursor] = ch;
                        cursor++;
                    }
            }
    }
}

static void get_screen_size(void) {
    struct message m;
    m.type = TEXTSCREEN_GET_SIZE_MSG;

    error_t err = ipc_call(display_server, &m);
    ASSERT_OK(err);
    ASSERT(m.type == TEXTSCREEN_GET_SIZE_REPLY_MSG);

    height = m.textscreen_get_size_reply.height;
    width = m.textscreen_get_size_reply.width;
}

void main(void) {
    TRACE("starting...");

    display_server = ipc_lookup("display");
    ASSERT_OK(display_server);

    kbd_server = ipc_lookup("kbd");
    ASSERT_OK(kbd_server);

    struct message m;
    m.type = KBD_LISTEN_MSG;
    ipc_call(kbd_server, &m);

    get_screen_size();
    clear_screen();

    // The mainloop: receive and handle messages.
    prompt();
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                if (m.notifications.data & NOTIFY_ASYNC) {
                    async_recv(kbd_server, &m);
                    switch (m.type) {
                        case KBD_ON_KEY_UP_MSG:
                            input(m.kbd_on_key_up.keycode);
                            break;
                        default:
                            WARN("unknown message type (type=%d)", m.type);
                            break;
                    }
                }
                break;
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
