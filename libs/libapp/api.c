#include <resea_idl.h>
#include <app.h>
#include <resea.h>

void api_console_write(UNUSED const char *s) {
    while (*s) {
        TRY_OR_PANIC(console_write(2 /* gui_ch */, *s));
        s++;
    }
}
