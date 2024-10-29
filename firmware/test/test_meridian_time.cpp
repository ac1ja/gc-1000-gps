#include <Arduino.h>
#include <unity.h>
#include <Adafruit_I2CDevice.h> // Required because the RTC lib doesn't sanitize library headers

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
    // Midnight should read 1 in 24hr mode

    int _utc_hour = 0;
    TEST_ASSERT_EQUAL(1, meridianTime(_utc_hour, true));
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

    UNITY_END();
}

void loop()
{
    // PlatformIO handles execution
}