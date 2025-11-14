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
#include <ArduinoLog.h>
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include <RTClib.h>          // https://github.com/adafruit/RTClib
#include <TimeLib.h>         // https://github.com/PaulStoffregen/Time
#include <TimerOne.h>        // https://github.com/PaulStoffregen/TimerOne
#include <Timezone.h>        // https://github.com/JChristensen/Timezone
#include <TinyGPSPlus.h>
#include <avr/wdt.h>

// Our libs
#include "display.h"

// Our headers
#include "boardConfig.h"
#include "buildData.h"
#include "constants.h"
#include "timezones.h"

// Display
Display display(SEGMENT_ENABLE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

// Hi-Spec and time age
unsigned long lastTimeSync;   // How long ago was time set
volatile bool hasTimeBeenSet; // Has the time been set

// PPS sync flag
volatile bool pps = 0;

// Time and time vars
uint8_t storedMonth, storedDay, storedHour, storedMinute, storedSecond,
    storedHundredths, storedTenths;
int16_t storedYear;
uint32_t storedAge;      // same as fixed_afe in docs, time since fix.
unsigned long last_loop; // Used for calculating loop time
volatile bool syncReady;
volatile byte lastMinute;
byte lastTimezoneCheckMinute = 60;

// Dip switch settings
bool newSettingsFlag = false; // true whenever settings have been changed
unsigned int DIPsum; // holds the sum value of all switches to check for when
                     // settings are changed
const byte TimeZoneInputs[] = {
    DIP0, DIP1, DIP2, DIP3, DIP4}; // what pins to use for the time zone inputs
int16_t timeZone;                  // the current timezone
const byte ClockFormatInput =
    DIP2;                       // what pin to use to check if 24 or 12hr format
const byte LocalTZInput = DIP0; // what pin to use to check if we're using UTC
                                // or local TZ (TimeZoneInputs)
const byte ObserveDSTInput =
    DIP3; // what pin to use to check if observing DST or not
bool isUsingLocalTZInput =
    true; // whether or not we're currently using the local tz input
bool isObservingDST = true; // whether or not we're observing DST

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

bool flasher() { return (millis() / 400) % 2; }

void isrPPS() {
  // flag the 1pps input signal
  pps = true;

  // if minute has changed...allow a GPS sync to happen
  if (lastMinute != minute())
    hasTimeBeenSet = false; // Might need to refactor this? seems redundant?
}

bool isHighSpec() {
  return (millis() - lastTimeSync < hiSpecMaxAge) && hasTimeBeenSet;
}

void applyUtcOffsetMinutes(int offsetMinutes) {
  int hourOffset = offsetMinutes / 60;
  int minuteOffset = offsetMinutes % 60;

  if (minuteOffset < 0) {
    minuteOffset += 60;
    hourOffset -= 1;
  }

  if (utcHourOffset != hourOffset || utcMinuteOffset != minuteOffset) {
    Log.infoln("UTC offset updated: %d hours, %d minutes (total %d).",
               hourOffset, minuteOffset, offsetMinutes);
  }

  utcHourOffset = hourOffset;
  utcMinuteOffset = minuteOffset;
}

void updateTimezoneOffsets(bool force) {
  static bool initialized = false;
  static int lastOffsetMinutes = 0;
  static bool lastDstState = false;

  if (!isUsingLocalTZInput) {
    if (!initialized || force || utcHourOffset != 0 || utcMinuteOffset != 0) {
      applyUtcOffsetMinutes(0);
      lastOffsetMinutes = 0;
      lastDstState = false;
      initialized = true;
    }
    return;
  }

  TimeChangeRule localDST = {"DST", Second, Sun, Mar, 2, timeZone};
  TimeChangeRule localSTD = {"STD", First, Sun, Nov, 2, timeZone - 60};
  Timezone tz(localDST, localSTD);

  bool dstActive = isObservingDST && tz.utcIsDST(now());
  int offsetMinutes = dstActive ? localDST.offset : localSTD.offset;

  if (!initialized || force || offsetMinutes != lastOffsetMinutes) {
    applyUtcOffsetMinutes(offsetMinutes);
    Log.verboseln(F("Offset recalculated: raw minutes %d, hour offset %d, "
                    "minute offset %d"),
                  offsetMinutes, utcHourOffset, utcMinuteOffset);
    lastOffsetMinutes = offsetMinutes;
  }

  if (initialized && lastDstState != dstActive) {
    Log.noticeln("DST state changed from %s to %s.",
                 lastDstState ? "DST" : "STD", dstActive ? "DST" : "STD");
  }

  lastDstState = dstActive;
  initialized = true;
}

void pullRTCTime() {
  DateTime _now = rtc.now();
  // Set time using old RTC value.
  setTime(_now.hour(), _now.minute(), _now.second(), _now.day(), _now.month(),
          _now.year());
}

void syncCheck() {
  // Checks the PPS flag, limits us to doing only one syncCheck per second.
  if (pps) {
    // syncReady means that the GPS has a valid time and will wait for the PPS
    // to trigger the precise second hasTimeBeenSet tells us if the time has
    // been set at all (cold start) isHighSpec() tells us if the time is valid
    // and known-good (High Accuracy)
    if (syncReady && (!hasTimeBeenSet || !isHighSpec())) {
      // Compute Drift
      // byte drift = storedSecond - rtc.now().second();
      int drift = storedSecond - second() - 1;
      syncReady = false; // Reset syncReady flag

      // Only bother adjusting the time if needed (issue #18)
      if (drift == 0) {
        // Leverages the internal time system for fast time access
        setTime(storedHour, storedMinute, storedSecond, storedDay, storedMonth,
                storedYear);
        // adjustTime(1); // 1pps signal = start of next second
      }

      rtc.adjust(DateTime(storedYear, storedMonth, storedDay, storedHour,
                          storedMinute, storedSecond));
      lastTimeSync = millis();
      hasTimeBeenSet = true; // Time has been set
      updateTimezoneOffsets(true);

      // Display Drift
      if (abs(drift) < 0) {
        display.setDrift(display.SLOW);
      } else if (drift > 0) {
        display.setDrift(display.FAST);
      } else {
        display.setDrift(display.NONE);
      }

      Log.infoln("Synced! Drift was %d seconds", drift);
    } else {
      Log.warningln("PPS triggered but not ready for sync!");
    }

    pps = false;
  } else {
    // Might be nice to move this to a more periodic function if we go the RTOS
    // route.
    pullRTCTime();
  }

  lastMinute = storedMinute; // Last minute is now the stored minute
}

void updateBoard(void) {
  // read the status of comm pins
  display.setData(
      !digitalRead(debugSerialCheck)); // if there is data on the serial line
  display.setCapture(
      !digitalRead(gpsSerialCheck)); // if the gps is being read from
  display.setHighSpec(
      isHighSpec()); // if the time has been locked in/synced to the rtc

  // Local var use24mode (could be refactored)
  bool _use24mode = clockFormat == 24;

  display.setDispTime(
      isUsingLocalTZInput ? meridianTime(getUTCOffsetHours(hour()), _use24mode)
                          : meridianTime(hour(), _use24mode),
      isUsingLocalTZInput ? getUTCOffsetMinutes(minute()) : minute(), second(),
      isHighSpec() ? (((millis() - lastTimeSync) / 100) % 10)
      : flasher()  ? 99
                   : satsInView,
      _use24mode);

  // Setting the AM/PM lights
  // Log.verbose(F("Setting the AM/PM lights for %d" CR), hour());

  if (!_use24mode) {
    display.setMeridan(getAM(getUTCOffsetHours(hour())),
                       !getAM(getUTCOffsetHours(hour())));
  } else {
    display.setMeridan(false, false);
  }

  display.updateBoard();
}

void setup() {
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

  enableInterrupt(GPS_PPS_PIN, isrPPS,
                  RISING); // Attach interrupt to gps PPS pin
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
  Timer1.initialize(3000); // Cycle every 3000μs
  Timer1.attachInterrupt(
      updateBoard); // Attach an interrupt to callback updateBoard()

  // Quick-load rtc time at boot
  pullRTCTime();

  // Set last loop
  last_loop = millis();
}

void loop() {
  // if we have not yet set the time OR if the current time is out of date
  if (!hasTimeBeenSet || !isHighSpec()) {
    unsigned long _start = millis();

    while (Serial3.available()) {
      char c = Serial3.read();
      // Serial.print(c);
      if (gps.encode(c)) { // process gps messages
        // new data...let's crack the date/time
        if (gps.time.isValid()) {
          storedHour = gps.time.hour();
          storedMinute = gps.time.minute();
          storedSecond = gps.time.second();
          storedTenths = gps.time.centisecond();
          storedAge = gps.time.age();
          satsInView = gps.satellites.value();

          Log.verbose(
              F("Cracked a new time! Time is %d:%d:%d.%d, Age is %d" CR),
              storedHour, storedMinute, storedSecond, storedTenths, storedAge);
        } else {
          Log.warningln("Time is invalid? Could not crack!");
        }

        if (storedAge < 1000) {
          // it's good data (not old)...so, let's use it
          Log.verboseln("Age is good! Setting sync ready flag! %d -> %d, pps "
                        "is %d, numsats %d",
                        syncReady, true, pps, satsInView);
          syncReady = true;
        } else {
          Log.warningln("Could not set time: Data too old");
        }

        syncCheck();
        Log.verboseln("Cracking GPS packet took %dms.", millis() - _start);
        break;
      }
    }
  }

  // check if its time to check for the dip settings TODO: move this to a
  // scheduled task, see branch task-scheduler
  if (dipcheck++ >
      400) { // check if 400 cycles have passed since last updating the dips
    // check if any settings have changed since last time
    unsigned int _DIPsum =
        0;             // create a temporary place to store out dipswitch values
    byte DIPA = ~PINA; // ~ to invert
    byte DIPC = ~PINC; // ~ to invert
    _DIPsum = DIPA + DIPC;

    // if any of the switches were changed, update everything
    if (_DIPsum != DIPsum || newSettingsFlag) {
      Log.verbose(F("Updating dip switches, DIPA set to %b, DIPC set to %b" CR),
                  DIPA, DIPC);

      // update timezone
      unsigned int _timeZone = 0; // clearout a temporary int of memory
      for (byte i = 0; i < sizeof TimeZoneInputs / sizeof TimeZoneInputs[0];
           i++) { // read every byte in the dipswitch list
        int value = bitRead(DIPC, TimeZoneInputs[i]); // read byte (0001, 0000)
        _timeZone =
            _timeZone + (value << i); // shif byte to its correct magnitude
      }

      timeZone =
          (_timeZone - 12) * 60; // store the new timezone value, offset by -12
                                 // (so we dont need to use a signed dip switch)

      // update clock format
      if (bitRead(DIPA, ClockFormatInput)) {
        clockFormat = 24;
      } else {
        clockFormat = 12;
      }

      // Updates the flag letting us know if we're using localtz or not
      isUsingLocalTZInput = !bitRead(DIPA, LocalTZInput);

      // Updates if we're observing dst or not
      isObservingDST = bitRead(DIPA, ObserveDSTInput); // not used?

      updateTimezoneOffsets(true);

      // Reset flags and sums
      newSettingsFlag = false;
      DIPsum = _DIPsum;
    }
    dipcheck = 0; // Return dipcheck to 0
  }

  // Trigger Watchdog
  wdt_reset();

  if (hasTimeBeenSet) {
    byte currentMinute = minute();
    if (currentMinute != lastTimezoneCheckMinute) {
      lastTimezoneCheckMinute = currentMinute;
      updateTimezoneOffsets(false);
    }
  }

  unsigned long _dur = millis() - last_loop;
  last_loop = millis();
  if (_dur > 50) {
    Log.warningln("Main Loop overrun, took %dms.", _dur);
  }
}
