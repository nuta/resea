#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/timer.h>
#include <string.h>
#include "datetime.h"

static uint64_t timeofday;

void init_datetime(struct datetime *datetime) {
    task_t rtc_driver = ipc_lookup("rtc");
    struct message m;
    m.type = RTC_READ_MSG;
    ASSERT_OK(ipc_call(rtc_driver, &m));
    datetime->year = m.rtc_read_reply.year;
    datetime->month = m.rtc_read_reply.month;
    datetime->day = m.rtc_read_reply.day;
    datetime->day_of_week = m.rtc_read_reply.day_of_week;
    datetime->hour = m.rtc_read_reply.hour;
    datetime->minute = m.rtc_read_reply.minute;
    datetime->second = m.rtc_read_reply.second;
}

void main(void) {
    TRACE("starting...");
    struct datetime datetime;
    init_datetime(&datetime);
    timeofday = gettimestamp(datetime);

    ASSERT_OK(ipc_serve("datetime"));

    INFO("ready");
    timer_set(1000 /* in milliseconds */);
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case NOTIFICATIONS_MSG: {
                if (m.notifications.data & NOTIFY_TIMER) {
                    timeofday++;
                    timer_set(1000);
                }
                break;
            }
            case TIME_GETTIMEOFDAY_MSG: {
                m.type = TIME_GETTIMEOFDAY_REPLY_MSG;
                m.time_gettimeofday_reply.unixtime = timeofday;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
