#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/datetime.h>
#include <string.h>
#include "test.h"

#define TEST_START_TIME 1577836800 // January 1 2020 12:00:00 AM
#define TEST_END_TIME   1577923200 // January 2 2020 12:00:00 AM

void datetime_test(void) {
    uint64_t timestamp = TEST_START_TIME;
    struct datetime datetime;
    timestamp_to_datetime(timestamp, &datetime);

    while (timestamp <= TEST_END_TIME) {
        timestamp_to_datetime(timestamp, &datetime);
        TEST_ASSERT(datetime.second >= 0 && datetime.second < 60);
        TEST_ASSERT(datetime.minute >= 0 && datetime.minute < 60);
        TEST_ASSERT(datetime.hour >= 0 && datetime.hour < 24);
        TEST_ASSERT(datetime.month == 1);
        TEST_ASSERT(datetime.year == 2020);
        ++timestamp;
    }

    // Test for start of epoch time
    timestamp = 0;
    timestamp_to_datetime(timestamp, &datetime);
    TEST_ASSERT(datetime.day == 1);
    TEST_ASSERT(datetime.month == 1);
    TEST_ASSERT(datetime.year == 1970);

    // Test datetime struct one day after epoch
    datetime.second = 0;
    datetime.minute = 0;
    datetime.hour = 0;
    datetime.day = 2;
    datetime.month = 1;
    datetime.year = 1970;
    timestamp = datetime_to_timestamp(&datetime);
    TEST_ASSERT(timestamp == 86400);
}
