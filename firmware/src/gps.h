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
volatile int pps = 0;

// Time and time vars
uint8_t storedMonth, storedDay, storedHour, storedMinute, storedSecond, storedHundredths;
int16_t storedYear;
uint32_t storedAge; // same as fixed_afe in docs, time since fix.
volatile bool syncReady;
volatile byte lastMinute;

void TaskGPSCommunicate(void *pvParameters)
{
    // Suspend/delay for 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);

    if (!hasTimeBeenSet)
    { // if we have not yet set the time
        // Serial.print("Num satelites: "); Serial.println(gps.satellites());
        // Serial.println("Attempting top set time");
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
            } // else {Serial.println("Could not set time, no data to read");}
            // syncCheck(); // check for a sync after attempting to crack a new data stream TODO: we may not need to check here if the last crack failed.
        }
        // syncCheck(); // check for a sync after we've finished running through all available GPS data TODO: we may not need to check here if the last crack failed.
    }
}
