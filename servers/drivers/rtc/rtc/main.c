#include "rtc.h"
#include <driver/io.h>
#include <resea/datetime.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <vprintf.h>

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

void read_datetime(struct datetime *datetime) {
    // Wait until rtc is not updating
    while (is_update_in_progress())
        ;

    datetime->second = read_cmos_register(0x00);
    datetime->minute = read_cmos_register(0x02);
    datetime->hour = read_cmos_register(0x04);
    datetime->day_of_week = read_cmos_register(0x06);
    datetime->day = read_cmos_register(0x07);
    datetime->month = read_cmos_register(0x08);
    datetime->year = read_cmos_register(0x09);

    uint8_t status_b = read_cmos_register(0x0B);
    bool use_bcd = !(status_b & 0x04);
    bool use_twelve_hours = !(status_b & 0x02);

    if (use_bcd) {
        datetime->second = bcd_to_binary(datetime->second);
        datetime->minute = bcd_to_binary(datetime->minute);
        datetime->hour = bcd_to_binary(datetime->hour & 0x7F);
        datetime->day_of_week = bcd_to_binary(datetime->day_of_week);
        datetime->day = bcd_to_binary(datetime->day);
        datetime->month = bcd_to_binary(datetime->month);
        datetime->year = bcd_to_binary(datetime->year);
    }

    datetime->year += 2000;

    if (use_twelve_hours && (datetime->hour & 0x80)) {
        datetime->hour = (((datetime->hour & 0x7F) + 12) % 24);
    }
}

void main(void) {
    rtc_io = io_alloc_port(RTC_PORT_IDX, 0x02, IO_ALLOC_NORMAL);

    ASSERT_OK(ipc_serve("rtc"));

    TRACE("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case RTC_READ_MSG: {
                struct datetime datetime;
                read_datetime(&datetime);

                m.type = RTC_READ_REPLY_MSG;
                m.rtc_read_reply.year = datetime.year;
                m.rtc_read_reply.month = datetime.month;
                m.rtc_read_reply.day = datetime.day;
                m.rtc_read_reply.day_of_week = datetime.day_of_week;
                m.rtc_read_reply.hour = datetime.hour;
                m.rtc_read_reply.minute = datetime.minute;
                m.rtc_read_reply.second = datetime.second;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
