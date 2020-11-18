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

uint64_t datetime_to_timestamp(struct datetime *datetime);
void timestamp_to_datetime(uint64_t epoch, struct datetime *datetime);

#endif
