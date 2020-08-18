#include <resea/printf.h>
#include <resea/cmdline.h>
#include <string.h>
#include "http.h"

void main(const char *cmdline) {
    char *cmd, *url;
    cmdline_cmd("http-get");
    cmdline_arg(&url, "http-get", "url", false);
    cmdline_parse(cmdline, &cmd);

    if (!strcmp(cmd, "http-get")) {
        http_get(url);
    }
}
