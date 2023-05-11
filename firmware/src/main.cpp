/**
 * @brief arduinio controlled Heathkit GC-1000 clock display driver
 * licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
 *
 * Nick Soggu
 * created march 2020
 * revised for arduino mega on 2021
 */

#include "Arduino.h"
#include "avr8-stub.h"

// Libs, see platformio.ini
#include <Arduino_FreeRTOS.h>
#include <RTClib.h>          // https://github.com/adafruit/RTClib
#include <TimeLib.h>         // https://github.com/PaulStoffregen/Time
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include <TimerOne.h>        // https://github.com/PaulStoffregen/TimerOne
#include <avr/wdt.h>
#include <ArduinoLog.h>

// Our libs
#include "display.h"

// Our headers
#include "boardConfig.h"
#include "buildData.h"
#include "timezones.h"
#include "gps.h"
#include "switches.h"

// Display
Display display(SEGMENT_ENABLE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

// display lights
const byte debugSerialCheck = 1; // debug activity pin
const byte gpsSerialCheck = 15;  // gps activity pin

// hardware objects
RTC_DS3231 rtc;

time_t prevDisplay = 0; // when the digital clock was displayed

byte pos = 0;
bool AM, PM;

byte localHour, localMinute, localSecond, localTens;
byte dataLED, captureLED, highSpecLED = false;

bool mhz5, mhz10, mhz15;

void isrPPS()
{
  // flag the 1pps input signal
  pps = 1;

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

void updateBoard(void)
{
  // read the status of comm pins
  display.setData(!digitalRead(debugSerialCheck));             // if there is data on the serial line
  display.setCapture(!digitalRead(gpsSerialCheck));            // if the gps is being read from
  display.setHighSpec(millis() - lastTimeSync < hiSpecMaxAge); // if the time has been locked in/synced to the rtc

  display.setDispTime(getUTCOffsetHours(hour()),
                      getUTCOffsetMinutes(minute()),
                      second(),
                      (((millis() - lastTimeSync) / 100) % 10));

  display.setMeridan(getAM(hour()), !getAM(hour()));

  display.updateBoard();
}

void setup()
{
  // initalize Serial interfaces
  Serial.begin(115200); // USB (debug)
  Serial3.begin(9600);  // GPS

  // Setup logging
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.noticeln("Serial started.");

  // inatialize input pins
  pinMode(GPS_PPS_PIN, INPUT); // GPS PPS signal

  // initalize I/O pins
  DDRA = 0x00;
  PORTA = 0xFF; // ALl PORTA pins HIGH
  DDRC = 0x00;
  PORTC = 0xFF; // ALl PORTC pins HIGH
  pinMode(debugSerialCheck, INPUT);
  pinMode(gpsSerialCheck, INPUT);
  Log.noticeln("Initalized all I/O pins");

  // start rtc
  rtc.begin();
  Log.noticeln("Started RTC.");

  lastTimeSync = millis() + hiSpecMaxAge;
  hasTimeBeenSet = false;

  // clear screen
  // dataOut = 0;
  // shiftOut(DATA_PIN, CLOCK_PIN, dataOut);

  enableInterrupt(GPS_PPS_PIN, isrPPS, RISING); // Attach interrupt to gps PPS pin
  Log.noticeln("Initalized all inturrupts");

  // print out some information about the software we're running.
  Log.noticeln(MOTD);

  // don't sync the time yet...
  syncReady = false;
  hasTimeBeenSet = false;
  lastMinute = -1;

  // Configure watchdog
  wdt_enable(WDTO_2S);

  // Initialize interrupts
  Timer1.initialize(3000);             // Cycle every 3000μs
  Timer1.attachInterrupt(updateBoard); // Attach an interrupt to callback updateBoard()

  // Configure Tasks
  xTaskCreate(
      TaskReadSwitches, "ReadSwitches",
      128,     // Stack size
      NULL, 2, // Priority
      NULL);

  xTaskCreate(
      TaskGPSCommunicate, "GPSCommunicate",
      128,     // Stack size
      NULL, 2, // Priority
      NULL);
}

void loop()
{
  // Trigger Watchdog TODO: Move me to an idle task!
  wdt_reset();
}
