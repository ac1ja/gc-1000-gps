#ifndef TIMEZONES_H
#define TIMEZONES_H

// uses timezone lib
// format is
// TimeChangeRule myRule = {abbrev, week, dow, month, hour, offset};

int utcHourOffset, utcMinuteOffset; // these are calculated at the speed we check the dip switches
byte clockFormat = 24;              // 12 or 24, (could be a bool but all code uses it with % so this is less work)

/**
 * @brief Return the bounded local 24hr time using the utcHourOffset
 * Value is always bounded between 00:00 and 23:59
 *
 * @param utc_hour Hour 0 - 23
 * @return int Hour 0 - 23
 */
int getUTCOffsetHours(byte utc_hour)
{
    // The UTC hour, plus our hour offset (-12 to +12), mod 24
    int local_hour = (utc_hour + utcHourOffset) % 24;
    return local_hour;
}

/**
 * @brief Return the minute offset (for the rare timezones that require that)
 *
 * @param utc_minute 0 - 59
 * @return int Minute 0 - 59
 */
int getUTCOffsetMinutes(byte utc_minute)
{
    byte local_minute = (utc_minute + utcMinuteOffset) % 60;

    return local_minute;
}

/**
 * @brief Checks if AM or not using LOCAL TIME
 *
 * @param local_hour 0 - 23
 * @return true when it is AM (local tz)
 * @return false when it is PM (local tz)
 */
bool getAM(byte local_hour)
{
    if (local_hour + utcHourOffset < 12)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Converts raw UTC 24-hour time to 12-hour or 24-hour format.
 *
 * @param hour (0 - 23) Input hour in 24-hour format.
 * @param use_24hr_format Set to true for 24-hour format, false for 12-hour format.
 * @return int Converted hour in either 12-hour (1 - 12) or 24-hour (1 - 23) format.
 */
int meridianTime(int hour, bool use_24hr_format = false)
{
    // Handle edge cases
    if (hour < 0 || hour > 23)
    {
        return 0;
    }

    // If using 24-hour format and hour is 0, return 1
    if (use_24hr_format)
    {
        return hour;
    }

    // Convert to 12-hour format
    if (hour == 0)
        return 12; // Midnight as 12 AM
    return hour > 12 ? hour - 12 : hour;
}

#endif
