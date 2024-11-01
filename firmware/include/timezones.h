// uses timezone lib
// format is
// TimeChangeRule myRule = {abbrev, week, dow, month, hour, offset};

int utcHourOffset, utcMinuteOffset; // these are calculated at the speed we check the dip switches
byte clockFormat = 24;              // 12 or 24, (could be a bool but all code uses it with % so this is less work)

int getUTCOffsetHours(byte utc_hour)
{
    /* return a value in hours offset from utc-0
     */

    byte local_hour = (utc_hour + utcHourOffset) % (clockFormat);

    // Clocks dont display 0:00
    if (local_hour == 0)
    {
        local_hour = 12;
    }

    return local_hour;
}

int getUTCOffsetMinutes(byte utc_minute)
{
    /* Not implemented!
     */

    byte local_minute = utc_minute + utcMinuteOffset;

    return local_minute;
}

bool getAM(byte utc_hour)
{
    /* Return true if
     * local time is AM
     */

    if (utc_hour + utcHourOffset < 12)
    {
        return true;
    }
    else
    {
        return false;
    }
}