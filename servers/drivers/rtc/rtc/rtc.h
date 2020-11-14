#ifndef __RTC_H__
#define __RTC_H__

#define RTC_PORT_IDX  0x70

struct datetime {
    uint32_t year;
    uint8_t month;
    uint8_t day;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

#endif
