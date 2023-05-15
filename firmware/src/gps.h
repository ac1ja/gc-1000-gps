/**
 * @file gps.h
 * @author Joe
 * @brief Methods and tasks related to the gps live here.
 */

#include "Arduino.h"
#include <Arduino_FreeRTOS.h>
#include <TinyGPS.h> // https://github.com/mikalhart/TinyGPS
#include <ArduinoLog.h>

// hardware objects
TinyGPS gps;

// Hi-Spec and time age
const uint16_t hiSpecMaxAge = 60000; // 60 Seconds
unsigned long lastTimeSync;          // How long ago was time set
volatile bool hasTimeBeenSet;        // Has the time been set

// PPS sync flag
volatile int pps, displayPPS = 0;

// Time and time vars
uint8_t storedMonth, storedDay, storedHour, storedMinute, storedSecond, storedHundredths;
int storedYear;
unsigned long storedAge; // same as fixed_afe in docs, time since fix.
volatile bool syncReady;
volatile byte lastMinute;

void isrPPS()
{
    pps = 1;        // Flag signaling that we've hit the PPS
    displayPPS = 0; // Used for the visual display side, will take longer to be picked up and reset by other tasks

    // if minute has changed...allow a GPS sync to happen
    if (lastMinute != minute())
        hasTimeBeenSet = false;
}

void syncCheck()
{
    if (pps)
    {
        if (syncReady && !hasTimeBeenSet)
        {
            setTime(storedHour, storedMinute, storedSecond, storedDay, storedMonth, storedYear); // Set the time? This puts this time in local
            rtc.adjust(DateTime(storedYear, storedMonth, storedDay, storedHour, storedMinute, storedSecond));

            // Not great but maybe could be improved, this whole time-critical section needs to be simplified, timezones make it so hard.
            byte drift = storedSecond - rtc.now().second();
            if (abs(drift) < 1)
            {
                display.setDrift(display.NONE);
            }
            else if (drift > 1)
            {
                display.setDrift(display.SLOW);
            }
            else
            {
                display.setDrift(display.FAST);
            }

            // rtc.adjust(dipTZ.toLocal(DateTime(storedYear, storedMonth, storedDay, storedHour, storedMinute, storedSecond).unixtime())); // adjust the time to the tz.tolocal conversion of gps stored data
            adjustTime(1); // 1pps signal = start of next second
            lastTimeSync = millis();
            hasTimeBeenSet = true;     // Time has been set
            newSettingsFlag = true;    // New settings are in place
            syncReady = false;         // Reset syncReady flag
            lastMinute = storedMinute; // Last minute is now the stored minute

            Log.verboseln("Synced!");
        }
    }
    pps = 0;
}

void TaskGPSCommunicate(void *pvParameters)
{
    for (;;)
    { // Run forever

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (!hasTimeBeenSet)
        { // if we have not yet set the time
            Log.verboseln("Num satellites: %d", gps.satellites());
            while (Serial3.available())
            {
                if (gps.encode(Serial3.read()))
                { // process gps messages
                    // new data...let's crack the date/time
                    gps.crack_datetime(&storedYear, &storedMonth, &storedDay, &storedHour, &storedMinute, &storedSecond, &storedHundredths, &storedAge);
                    Log.verbose(F("Cracked a new time! Second is %d, Age is %d" CR), storedSecond, storedAge);
                    if (storedAge < 1000)
                    {
                        // it's good data (not old)...so, let's use it
                        syncReady = true;
                    }
                    else
                    {
                        Log.warningln("Could not set time, Data too old");
                    }
                }            // else {Serial.println("Could not set time, no data to read");}
                syncCheck(); // check for a sync after attempting to crack a new data stream TODO: we may not need to check here if the last crack failed.
            }
            syncCheck(); // check for a sync after we've finished running through all available GPS data TODO: we may not need to check here if the last crack failed.
        }
    }
}

void TaskBlinkOnPPS(void *pvParameters)
{
    for (;;)
    { // Task will never end

        // Wait for 100ms
        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (displayPPS == 1)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            displayPPS = 0;
        }
    }
}