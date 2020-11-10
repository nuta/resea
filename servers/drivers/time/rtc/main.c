#include <resea/printf.h>
#include <vprintf.h>
#include <driver/io.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <string.h>
#include "rtc.h"

static io_t rtc_io;


void main(void) {
    rtc_io = io_alloc_port(RTC_PORT_IDX, 0x10, IO_ALLOC_NORMAL);

    ASSERT_OK(ipc_serve("time"));

    TRACE("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case TIME_READ_MSG: {
                char *format = m.time_read.format;
                DBG("Received format -> %s", format);
                m.type = TIME_READ_REPLY_MSG;
                io_write8(rtc_io, 0, 0);
                char sec = io_read8(rtc_io, 1);
                io_write8(rtc_io, 0, 2);
                char min = io_read8(rtc_io, 1);
                io_write8(rtc_io, 0, 4);
                char hour = io_read8(rtc_io, 1);
                int buf_size = 1024;
                char *buf = malloc(buf_size);
                snprintf(buf, buf_size, "%02x:%02x:%02x", hour, min, sec);
                // DBG("TIME -> %02x:%02x:%02x", hour, min, sec);
                m.time_read_reply.currenttime = buf;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }

}
