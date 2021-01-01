#include <resea/datetime.h>
#include <resea/printf.h>
#include <types.h>

static bool is_leap_year(uint32_t year) {
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

uint64_t datetime_to_timestamp(struct datetime *datetime) {
    uint64_t seconds_since_epoch;
    uint32_t year;
    const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};

    // Convert years to no. of years since 1970.
    year = datetime->year - 1970;

    seconds_since_epoch = year * (365 * 86400);
    for (uint32_t i = 0; i < year; i++) {
        if (is_leap_year(i + 1970)) {
            seconds_since_epoch += 86400;
        }
    }

    for (uint32_t i = 1; i < datetime->month; i++) {
        if (is_leap_year(year + 1970) && i == 2) {
            seconds_since_epoch += (86400 * 29);
        } else {
            seconds_since_epoch += (86400 * days_in_month[i - 1]);
        }
    }

    seconds_since_epoch += ((datetime->day - 1) * 86400);
    seconds_since_epoch += (datetime->hour * 3600);
    seconds_since_epoch += (datetime->minute * 60);
    seconds_since_epoch += datetime->second;

    return seconds_since_epoch;
}

void timestamp_to_datetime(uint64_t epoch, struct datetime *datetime) {
    uint32_t year, month, day;
    const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};

    datetime->second = epoch % 60;
    epoch /= 60;
    datetime->minute = epoch % 60;
    epoch /= 60;
    datetime->hour = epoch % 24;
    epoch /= 24;
    datetime->day_of_week = ((epoch + 4) % 7) + 1;

    year = 1970;
    day = 0;
    while ((day += (is_leap_year(year) ? 366 : 365)) <= epoch) {
        year++;
    }
    datetime->year = year;

    day -= is_leap_year(year) ? 366 : 365;
    epoch -= day;

    day = 0;
    month = 0;
    for (month = 0; month < 12; month++) {
        uint8_t month_length = days_in_month[month];
        if (is_leap_year(year) && month == 1) {
            ++month_length;
        }
        if (epoch >= month_length) {
            epoch -= month_length;
        } else {
            break;
        }
    }

    datetime->month = month + 1;
    datetime->day = epoch + 1;
}
