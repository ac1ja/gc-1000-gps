int getUTCOffsetHours(byte utc_hour) {
    /* return a value in hours offset from utc-0
     */

    byte local_time = (utc_hour + (timeZone - 13)) % clockFormat; // the local time is the timezone dip setting offset by 13, added to the UTC hour, and % 24 or % 12 to fit it onto a clock display scale.

    return local_time;
}

int getUTCOffsetMinutes(byte utc_minute) {
    /* Not implemented!
     */

    return utc_minute;
}

bool getAM(byte utc_hour) {
    /* Return true if 
     * local time is AM
     */

    byte local_time = (utc_hour + (timeZone - 13)) % 24;

    if ((local_time % 12) <= 0) {
        return true;
    } else {
        return false;
    }
}
