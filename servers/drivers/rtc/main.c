#include <resea/printf.h>
#include <vprintf.h>
#include <driver/io.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <string.h>
#include "rtc.h"

static io_t rtc_io;

bool is_update_in_progress(void) {
	io_write8(rtc_io, 0, 10);
    uint8_t status = io_read8(rtc_io, 1);
	return status & 0x80;
}

uint8_t read_cmos_register(int reg_index) {
    io_write8(rtc_io, 0, reg_index);
    return (uint8_t) io_read8(rtc_io, 1);
}

uint8_t bcd_to_binary(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

void read_datetime(uint8_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute, uint8_t* second, uint8_t* day_of_week) {

    // Wait until rtc is not updating
    while (is_update_in_progress());

    *second = read_cmos_register(0x00);
    *minute = read_cmos_register(0x02);
    *hour = read_cmos_register(0x04);
    *day_of_week = read_cmos_register(0x06);
    *day = read_cmos_register(0x07);
    *month = read_cmos_register(0x08);
    *year = read_cmos_register(0x09);

    uint8_t status_b = read_cmos_register(0x0B);
    bool use_bcd = !(status_b & 0x04);
    bool use_twelve_hours = !(status_b & 0x02);

    if(use_bcd) {
        *second = bcd_to_binary(*second);
        *minute = bcd_to_binary(*minute);
        *hour = bcd_to_binary(*hour & 0x7F);
        *day_of_week = bcd_to_binary(*day_of_week);
        *day = bcd_to_binary(*day);
        *month = bcd_to_binary(*month);
        *year = bcd_to_binary(*year);
    }

    if(use_twelve_hours && (*hour & 0x80)) {
        *hour =(((*hour & 0x7F) + 12) % 24);
    }
}

void main(void) {
    rtc_io = io_alloc_port(RTC_PORT_IDX, 0x10, IO_ALLOC_NORMAL);

    ASSERT_OK(ipc_serve("rtc"));

    TRACE("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case TIME_READRTC_MSG: {
                char *format = m.time_readrtc.format;
                DBG("Received format -> %s", format);

                uint8_t year, month, day, hour, minute, second, day_of_week;
                read_datetime(&year, &month, &day, &hour, &minute, &second, &day_of_week);

                m.type = TIME_READRTC_REPLY_MSG;
                int buf_size = 1024;
                char *buf = malloc(buf_size);
                snprintf(buf, buf_size, "%d %d/%d/%d %d:%d:%d", day_of_week, day, month, year, hour, minute, second);
                m.time_readrtc_reply.currenttime = buf;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
