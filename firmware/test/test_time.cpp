#include <Arduino.h>
#include <unity.h>
#include <Adafruit_I2CDevice.h> // Required because the RTC lib doesn't sanitize library headers
#include <display.h>
#include "boardConfig.h"
#include <constants.h>

Display display(SEGMENT_ENABLE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

#include "timezones.h"

// 12-hour format tests
void test_12hr_format_midnight()
{
    // Midnight should read 12 AM in 12hr mode

    int _utc_hour = 0;
    TEST_ASSERT_EQUAL(12, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(true, getAM(_utc_hour));
}

void test_12hr_format_1_am()
{
    // 1 AM should read 1 in 12hr mode

    int _utc_hour = 1;
    TEST_ASSERT_EQUAL(1, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(true, getAM(_utc_hour));
}

void test_12hr_format_9_am()
{
    // 9 AM should read 9 in 12hr mode

    int _utc_hour = 9;
    TEST_ASSERT_EQUAL(9, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(true, getAM(_utc_hour));
}

void test_12hr_format_noon()
{
    // Noon should read 12 PM in 12hr mode

    int _utc_hour = 12;
    TEST_ASSERT_EQUAL(12, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

void test_12hr_format_1_pm()
{
    // 1 PM should read 1 in 12hr mode

    int _utc_hour = 13;
    TEST_ASSERT_EQUAL(1, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

void test_12hr_format_9_pm()
{
    // 9 PM should read 9 in 12hr mode

    int _utc_hour = 21;
    TEST_ASSERT_EQUAL(9, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

void test_12hr_format_11_pm()
{
    // 11 PM should read 11 in 12hr mode

    int _utc_hour = 23;
    TEST_ASSERT_EQUAL(11, meridianTime(_utc_hour, false));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

// 24-hour format tests
void test_24hr_format_midnight()
{
    // Midnight should read 0 in 24hr mode

    int _utc_hour = 0;
    TEST_ASSERT_EQUAL(0, meridianTime(_utc_hour, true));
    TEST_ASSERT_EQUAL(true, getAM(_utc_hour));
}

void test_24hr_format_1_am()
{
    // 1 AM should read 1 in 24hr mode

    int _utc_hour = 1;
    TEST_ASSERT_EQUAL(1, meridianTime(_utc_hour, true));
    TEST_ASSERT_EQUAL(true, getAM(_utc_hour));
}

void test_24hr_format_noon()
{
    // Noon should read 12 in 24hr mode

    int _utc_hour = 12;
    TEST_ASSERT_EQUAL(12, meridianTime(_utc_hour, true));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

void test_24hr_format_1_pm()
{
    // 1 PM should read 13 in 24hr mode

    int _utc_hour = 13;
    TEST_ASSERT_EQUAL(13, meridianTime(_utc_hour, true));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

void test_24hr_format_11_pm()
{
    // 11 PM should read 23 in 24hr mode

    int _utc_hour = 23;
    TEST_ASSERT_EQUAL(23, meridianTime(_utc_hour, true));
    TEST_ASSERT_EQUAL(false, getAM(_utc_hour));
}

// Edge/error cases
void test_12hr_format_28_pm()
{
    // If there were some rollover and 28 was passed to the function

    int _utc_hour = 28;
    TEST_ASSERT_EQUAL(0, meridianTime(_utc_hour, false));
}

void test_24hr_format_minus2_pm()
{
    // Negative ints are possible!

    int _utc_hour = -2;
    TEST_ASSERT_EQUAL(0, meridianTime(_utc_hour, true));
}

// UTC Offset tests
void test_no_offset()
{
    // No offset test

    int _current_time = 14;
    utcHourOffset = 0;
    TEST_ASSERT_EQUAL(14, getUTCOffsetHours(_current_time));
}

void setup()
{
    UNITY_BEGIN();

    // Run each test individually for 12-hour format
    RUN_TEST(test_12hr_format_midnight);
    RUN_TEST(test_12hr_format_1_am);
    RUN_TEST(test_12hr_format_9_am);
    RUN_TEST(test_12hr_format_noon);
    RUN_TEST(test_12hr_format_1_pm);
    RUN_TEST(test_12hr_format_9_pm);
    RUN_TEST(test_12hr_format_11_pm);

    // Run each test individually for 24-hour format
    RUN_TEST(test_24hr_format_midnight);
    RUN_TEST(test_24hr_format_1_am);
    RUN_TEST(test_24hr_format_noon);
    RUN_TEST(test_24hr_format_1_pm);
    RUN_TEST(test_24hr_format_11_pm);

    // Run individual tests for edge cases
    RUN_TEST(test_12hr_format_28_pm);
    RUN_TEST(test_24hr_format_minus2_pm);

    // UTC Offset tests
    RUN_TEST(test_no_offset);

    UNITY_END();

    // Done
    display.setCapture(true);
    display.setDrift(display.ALL);
    display.setHighSpec(true);
    display.setData(true);
}

void loop()
{
    display.setDispTime(88, 88, 88, 8);
    display.updateBoard();
    delay(2);
}