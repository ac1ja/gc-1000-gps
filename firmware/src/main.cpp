/**
 * @brief arduinio controlled Heathkit GC-1000 clock display driver
 * licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
 *
 * Nick Soggu
 * created march 2020
 * revised for arduino mega on 2021
 */

#include "Arduino.h"

// Libs, see platformio.ini
#include <TinyGPSPlus.h>
#include <RTClib.h>          // https://github.com/adafruit/RTClib
#include <TimeLib.h>         // https://github.com/PaulStoffregen/Time
#include <Timezone.h>        // https://github.com/JChristensen/Timezone
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include <TimerOne.h>        // https://github.com/PaulStoffregen/TimerOne
#include <avr/wdt.h>
#include <ArduinoLog.h>

// Our libs
#include "display.h"

// Our headers
#include "boardConfig.h"
#include "buildData.h"
#include "constants.h"
#include "timezones.h"

// Timezone
TimeChangeRule dipDST = {"DST", Second, Sun, Mar, 2, -240}; // Daylight time = UTC - 4 hours TODO: Changeme
TimeChangeRule dipSTD = {"STD", First, Sun, Nov, 2, -300};  // Standard time = UTC - 5 hours TODO: Changeme
Timezone dipTZ(dipDST, dipSTD);
TimeChangeRule *tcr; // pointer telling us where the TZ abbrev and offset is

// Display
Display display(SEGMENT_ENABLE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

// Hi-Spec and time age
unsigned long lastTimeSync;   // How long ago was time set
volatile bool hasTimeBeenSet; // Has the time been set

// PPS sync flag
volatile bool pps = 0;

// Time and time vars
uint8_t storedMonth, storedDay, storedHour, storedMinute, storedSecond, storedHundredths, storedTenths;
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
const byte LocalTZInput = DIP0;                               // what pin to use to check if we're using UTC or local TZ (TimeZoneInputs)
bool isUsingLocalTZInput = true;                              // Whether or not we're currently using the local tz input

long dipcheck = 0; // a counter to keep track of clock cycles before next update

// display lights
const byte debugSerialCheck = 1; // debug activity pin
const byte gpsSerialCheck = 15;  // gps activity pin

// hardware objects
TinyGPSPlus gps;
uint8_t satsInView = 0;
RTC_DS3231 rtc;

time_t prevDisplay = 0; // when the digital clock was displayed

byte pos = 0;
bool AM, PM;

byte localHour, localMinute, localSecond, localTens;
byte dataLED, captureLED, highSpecLED = false;

bool mhz5, mhz10, mhz15;

bool flasher()
{
  return (millis() / 400) % 2;
}

void isrPPS()
{
  // flag the 1pps input signal
  pps = true;

  // if minute has changed...allow a GPS sync to happen
  if (lastMinute != minute())
    hasTimeBeenSet = false; // Might need to refactor this? seems redundant?
}

bool isHighSpec()
{
  return (millis() - lastTimeSync < hiSpecMaxAge) && hasTimeBeenSet;
}

void pullRTCTime()
{
  DateTime _now = rtc.now();
  // Set time using old RTC value.
  setTime(_now.hour(), _now.minute(), _now.second(), _now.day(), _now.month(), _now.year());
}

void syncCheck()
{
  // Log.verboseln(F("Checking sync, pps %d, syncReady %d, hasTimeBeenSet %d, !isHighSpec %d"), pps, syncReady, !hasTimeBeenSet, !isHighSpec());

  // Checks the PPS flag, limits us to doing a syncCheck once per second.
  if (pps) // TODO: add a timeout here so we can sync even without pps signal
  {
    if (syncReady && (!hasTimeBeenSet || !isHighSpec()))
    {
      // Compute Drift
      // byte drift = storedSecond - rtc.now().second();
      int drift = storedSecond - second() - 1;

      setTime(storedHour, storedMinute, storedSecond, storedDay, storedMonth, storedYear); // Set the time? This puts this time in local
      rtc.adjust(DateTime(storedYear, storedMonth, storedDay, storedHour, storedMinute, storedSecond));

      // Display Drift
      if (abs(drift) < 0)
      {
        display.setDrift(display.SLOW);
      }
      else if (drift > 0)
      {
        display.setDrift(display.FAST);
      }
      else
      {
        display.setDrift(display.NONE);
      }

      // rtc.adjust(dipTZ.toLocal(DateTime(storedYear, storedMonth, storedDay, storedHour, storedMinute, storedSecond).unixtime())); // adjust the time to the tz.tolocal conversion of gps stored data
      adjustTime(1); // 1pps signal = start of next second

      // Skip updating the last time sync if not pps
      if (pps)
      {
        lastTimeSync = millis();
      }
      hasTimeBeenSet = true;     // Time has been set
      newSettingsFlag = true;    // New settings are in place
      syncReady = false;         // Reset syncReady flag
      lastMinute = storedMinute; // Last minute is now the stored minute

      Log.infoln("Synced! Drift was %d seconds", drift);
    }
    else
    {
      Log.warningln("PPS triggered but not ready for sync!");
    }

    pps = false;
  }
  else
  {
    // Might be nice to move this to a more periodic function if we go the RTOS route.
    pullRTCTime();
  }
}

void updateBoard(void)
{
  // read the status of comm pins
  display.setData(!digitalRead(debugSerialCheck));  // if there is data on the serial line
  display.setCapture(!digitalRead(gpsSerialCheck)); // if the gps is being read from
  display.setHighSpec(isHighSpec());                // if the time has been locked in/synced to the rtc

  display.setDispTime(getUTCOffsetHours(hour()),
                      getUTCOffsetMinutes(minute()),
                      second(),
                      isHighSpec() ? (((millis() - lastTimeSync) / 100) % 10) : flasher() ? 99
                                                                                          : satsInView);

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

  // initalize inturrupts
  Timer1.initialize(3000);             // Cycle every 3000μs
  Timer1.attachInterrupt(updateBoard); // Attach an interrupt to callback updateBoard()

  // Quick-load rtc time at boot
  pullRTCTime();
}

void loop()
{
  // if we have not yet set the time OR if the current time is out of date
  if (!hasTimeBeenSet || !isHighSpec())
  {
    while (Serial3.available())
    {
      char c = Serial3.read();
      // Serial.print(c);
      if (gps.encode(c))
      { // process gps messages
        // new data...let's crack the date/time
        if (gps.time.isValid())
        {
          storedHour = gps.time.hour();
          storedMinute = gps.time.minute();
          storedSecond = gps.time.second();
          storedTenths = gps.time.centisecond();
          storedAge = gps.time.age();
          satsInView = gps.satellites.value();

          Log.verbose(F("Cracked a new time! Time is %d:%d:%d.%d, Age is %d" CR), storedHour, storedMinute, storedSecond, storedTenths, storedAge);
        }
        else
        {
          Log.warningln("Time is invalid? Could not crack!");
        }

        if (storedAge < 1000)
        {
          // it's good data (not old)...so, let's use it
          Log.verboseln("Age is good! Setting sync ready flag! %d -> %d, pps is %d, numsats %d", syncReady, true, pps, satsInView);
          syncReady = true;
        }
        else
        {
          Log.warningln("Could not set time: Data too old");
        }
      }
      syncCheck(); // check for a sync after attempting to crack a new data stream TODO: we may not need to check here if the last crack failed.
    }
    syncCheck(); // check for a sync after we've finished running through all available GPS data TODO: we may not need to check here if the last crack failed.
  }

  // check if its time to check for the dip settings TODO: move this to a scheduled task, see branch task-scheduler
  if (dipcheck++ > 400)
  { // check if 400 cycles have passed since last updating the dips
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
      if (bitRead(DIPA, ClockFormatInput))
      {
        clockFormat = 24;
      }
      else
      {
        clockFormat = 12;
      }

      // Updates the flag letting us know if we're using localtz or not
      isUsingLocalTZInput = !bitRead(DIPA, LocalTZInput);

      // timezone
      // TODO: Most of these timezone values are HARDCODED until we find a way to easily craft dip switches that can read them.
      TimeChangeRule dipDST = {"DST", Second, Sun, Mar, 2, timeZone + 60}; // timezone offset (hrs) converted to minutes, offset by 1 hr
      TimeChangeRule dipSTD = {"STD", First, Sun, Nov, 2, timeZone};       // timezone offset (hrs) converted to minutes
      Timezone dipTZ(dipDST, dipSTD);

      dipTZ.toLocal(now(), &tcr); // setup local time (this can take thousands of cycles to compute)

      utcMinuteOffset = tcr->offset % 60;                   // strip out every full hour offset
      utcHourOffset = (tcr->offset - utcMinuteOffset) / 60; // the full hour offset

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
