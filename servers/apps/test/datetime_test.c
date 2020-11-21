#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/datetime.h>
#include <string.h>
#include "test.h"

void datetime_test(void) {
    uint64_t timestamp, timestamp_new;
    struct datetime datetime;

    // Test for random case
    // Friday, November 20, 2020 10:00:00 AM
    timestamp = 1605866400;
    timestamp_to_datetime(timestamp, &datetime);

    TEST_ASSERT(datetime.second == 0);
    TEST_ASSERT(datetime.minute == 0);
    TEST_ASSERT(datetime.hour == 10);
    TEST_ASSERT(datetime.day == 20);
    TEST_ASSERT(datetime.day_of_week == 6);
    TEST_ASSERT(datetime.month == 11);
    TEST_ASSERT(datetime.year == 2020);

    timestamp_new = datetime_to_timestamp(&datetime);
    TEST_ASSERT(timestamp_new == timestamp);

    // Test for leap year case
    // Saturday, February 29, 2020 12:34:56 PM
    timestamp = 1582979696;
    timestamp_to_datetime(timestamp, &datetime);

    TEST_ASSERT(datetime.second == 56);
    TEST_ASSERT(datetime.minute == 34);
    TEST_ASSERT(datetime.hour == 12);
    TEST_ASSERT(datetime.day == 29);
    TEST_ASSERT(datetime.day_of_week == 7);
    TEST_ASSERT(datetime.month == 2);
    TEST_ASSERT(datetime.year == 2020);

    timestamp_new = datetime_to_timestamp(&datetime);
    TEST_ASSERT(timestamp_new == timestamp);

    // Test for start of epoch time
    timestamp = 0;
    timestamp_to_datetime(timestamp, &datetime);

    TEST_ASSERT(datetime.second == 0);
    TEST_ASSERT(datetime.minute == 0);
    TEST_ASSERT(datetime.hour == 0);
    TEST_ASSERT(datetime.day == 1);
    TEST_ASSERT(datetime.day_of_week == 5);
    TEST_ASSERT(datetime.month == 1);
    TEST_ASSERT(datetime.year == 1970);

    timestamp_new = datetime_to_timestamp(&datetime);
    TEST_ASSERT(timestamp_new == timestamp);

    // Testing for end of epoch time is going to be time consuming.
    // Since we are using 64-bit timestamps.
}
