/**
 * @file switches.h
 * @author Joe
 * @brief Methods and tasks related to the dip switches live here.
 */

#include "Arduino.h"
#include <Arduino_FreeRTOS.h>
#include <ArduinoLog.h>

#include "boardConfig.h"

// Dip switch settings
bool newSettingsFlag = false;                                 // true whenever settings have been changed
unsigned int DIPsum;                                          // holds the sum value of all switches to check for when settings are changed
const byte TimeZoneInputs[] = {DIP0, DIP1, DIP2, DIP3, DIP4}; // what pins to use for the time zone inputs
int16_t timeZone;                                             // the current timezone
const byte ClockFormatInput = DIP2;                           // what pin to use to check if 24 or 12hr format

void TaskReadSwitches(void *pvParameters)
{
    for (;;)
    { // Run forever

        // Suspend/delay for 100ms
        vTaskDelay(100 / portTICK_PERIOD_MS);

        // check if any settings have changed since last time
        unsigned int _DIPsum = 0; // create a temporary place to store out dipswitch values
        byte DIPA = ~PINA;        // ~ to invert
        byte DIPC = ~PINC;        // ~ to invert
        _DIPsum = DIPA + DIPC;

        // if any of the switches were changed, update everything
        if (_DIPsum != DIPsum || newSettingsFlag)
        {
            Log.verbose(F("Updating dip switches, DIPA set to %b, DIPC set to %b" CR), DIPA, DIPC);

            // update timezone
            unsigned int _timeZone = 0; // clearout a temporary int of memory
            for (byte i = 0; i < sizeof TimeZoneInputs / sizeof TimeZoneInputs[0]; i++)
            {                                                 // read every byte in the dipswitch list
                int value = bitRead(DIPC, TimeZoneInputs[i]); // read byte (0001, 0000)
                _timeZone = _timeZone + (value << i);         // shif byte to its correct magnitude
            }

            timeZone = (_timeZone - 12) * 60; // store the new timezone value, offset by -12 (so we dont need to use a signed dip switch)

            // update clock format
            if (digitalRead(ClockFormatInput))
            {
                clockFormat = 24;
            }
            else
            {
                clockFormat = 12;
            }

            // timezone
            // TODO: Most of these timezone values are HARDCODED until we find a way to easily craft dip switches that can read them.
            // TimeChangeRule dipDST = {"DST", Second, Sun, Mar, 2, timeZone + 60}; // timezone offset (hrs) converted to minutes, offset by 1 hr
            // TimeChangeRule dipSTD = {"STD", First, Sun, Nov, 2, timeZone};       // timezone offset (hrs) converted to minutes
            // Timezone dipTZ(dipDST, dipSTD);

            // dipTZ.toLocal(now(), &tcr); // setup local time (this can take thousands of cycles to compute)

            // utcMinuteOffset = tcr->offset % 60;                   // strip out every full hour offset
            // utcHourOffset = (tcr->offset - utcMinuteOffset) / 60; // the full hour offset

            Log.verbose(F("Offset is %d, clock format is %d, utcHourOffset is %d" CR), _timeZone, clockFormat, utcHourOffset);

            Log.verboseln(F("UTC offset minutes is %d, minute is %d, minuteOffset is %d"), getUTCOffsetMinutes(minute()), minute(), utcMinuteOffset);

            // Reset flags and sums
            newSettingsFlag = false;
            DIPsum = _DIPsum;
        }
    }
}
