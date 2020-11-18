#ifndef __RESEA_DATETIME_H__
#define __RESEA_DATETIME_H__

struct datetime {
    uint32_t year;
    uint8_t month;
    uint8_t day;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

bool is_leap_year(uint32_t year);
uint64_t gettimestamp(struct datetime datetime);
void timestamp_to_datetime(uint64_t epoch, struct datetime *datetime);

#endif
