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
#include <TinyGPS.h>         // https://github.com/mikalhart/TinyGPS
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

// Dip switch settings
bool newSettingsFlag = false;                                 // true whenever settings have been changed
unsigned int DIPsum;                                          // holds the sum value of all switches to check for when settings are changed
const byte TimeZoneInputs[] = {DIP0, DIP1, DIP2, DIP3, DIP4}; // what pins to use for the time zone inputs
int16_t timeZone;                                             // the current timezone
const byte ClockFormatInput = DIP2;                           // what pin to use to check if 24 or 12hr format

long dipcheck = 0; // a counter to keep track of clock cycles before next update

// display lights
const byte debugSerialCheck = 1; // debug activity pin
const byte gpsSerialCheck = 15;  // gps activity pin

// hardware objects
TinyGPS gps;
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
      }            // else {Serial.println("Could not set time, no data to read");}
      syncCheck(); // check for a sync after attempting to crack a new data stream TODO: we may not need to check here if the last crack failed.
    }
    syncCheck(); // check for a sync after we've finished running through all available GPS data TODO: we may not need to check here if the last crack failed.
  }

  // check if its time to check for the dip settings TODO: move this to a scheduled task, see branch task-scheduler
  if (dipcheck++ > 80000)
  { // check if 80000 cycles have passed since last updating the dips
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
      {                                               // read every byte in the dipswitch list
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
    dipcheck = 0; // Return dipcheck to 0
  }

  // Trigger Watchdog
  wdt_reset();
}
