// arduinio controlled Heathkit GC-1000 clock display driver
// licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// Nick Soggu
// created march 2020
// revised for arduino mega on 2021

// arduino curated libraries (see requirements.txt)
#include <TinyGPS.h>          // https://github.com/mikalhart/TinyGPS
#include <RTClib.h>           // https://github.com/adafruit/RTClib
#include <TimeLib.h>          // https://github.com/PaulStoffregen/Time
#include <EnableInterrupt.h>  // https://github.com/GreyGnome/EnableInterrupt
#include <TimerOne.h>         // https://github.com/PaulStoffregen/TimerOne

// custom methods
#include "shiftOut.h"

// build information
#include "version.h"

// shift definitions
#define LATCH_PIN               8
#define CLOCK_PIN               12
#define DATA_PIN                11
#define SEGMENT_ENABLE_PIN      13

#define GPS_PPS_PIN             2

word dataOut = 0;

unsigned long lastMillis;
volatile bool hasTimeBeenSet;

volatile int pps = 0;  

volatile byte storedMonth, storedDay, storedHour, storedMinute, storedSecond, storedHundredths;                           
volatile int storedYear; 
volatile unsigned long storedAge;   
volatile bool syncReady;

volatile byte lastMinute;

// dip switch settings
const byte TimeZoneInputs[] = {53, 52, 51, 50, 49}; // what pins to use for the time zone inputs
const byte ClockFormatInput = 48; // what pin to use to check if 24 or 12hr format
byte clockFormat = 24; // 12 or 24, (could be a bool but all code uses it with % so this is less work)
unsigned int timeZone, _timeZone;
long dipcheck = 0;

// Hardware objects
TinyGPS gps;
RTC_DS3231 rtc;

time_t prevDisplay = 0; // when the digital clock was displayed

void setup() {
  // initalize Serial interfaces
  Serial.begin(115200); // USB (debug)
  Serial3.begin(9600);  // GPS

  // initalize output pins
  pinMode(LATCH_PIN, OUTPUT);           // Shift reg
  pinMode(CLOCK_PIN, OUTPUT);           // Shift reg
  pinMode(DATA_PIN, OUTPUT);            // Shift reg
  pinMode(SEGMENT_ENABLE_PIN, OUTPUT);  // Shift reg

  // inatialize input pins
  pinMode(GPS_PPS_PIN, INPUT);          // GPS PPS signal

  // initalize dip switch pins
  for (byte i = 0; i<sizeof TimeZoneInputs/sizeof TimeZoneInputs[0]; i++) {
    pinMode(TimeZoneInputs[i], INPUT);      // pin is input
    digitalWrite(TimeZoneInputs[i], HIGH);  // pin is pulldown
  }
  pinMode(ClockFormatInput, INPUT);
  digitalWrite(ClockFormatInput, HIGH);

  // Start rtc
  rtc.begin();

  lastMillis = millis();
  hasTimeBeenSet = false;

  // clear screen
  dataOut = 0;
  shiftOut(DATA_PIN, CLOCK_PIN, dataOut);

  // initalize inturrupts
  Timer1.initialize(3000); // Cycle every 3000μs
  Timer1.attachInterrupt(updateBoard); // Attach an interrupt to callback updateBoard()
  
  enableInterrupt(GPS_PPS_PIN, isrPPS, RISING);// Attach interrupt to gps PPS pin

  // print out some information about the software we're running.
  Serial.print("Starting gc-1000-gps software. Using version "); Serial.println(VERSION);
  Serial.print("This software compiled on "); Serial.println(COMPILED_ON); Serial.println();

  // don't sync the time yet...
  syncReady = false;
  hasTimeBeenSet = false;
  lastMinute = -1;
}

void isrPPS() {
  // flag the 1pps input signal
  pps = 1;

  // if minute has changed...allow a GPS sync to happen
  if (lastMinute != minute())
    hasTimeBeenSet = false;
}

void syncCheck() {
  if (pps) {
    if (syncReady && !hasTimeBeenSet) {
      setTime(storedHour, storedMinute, storedSecond, storedDay, storedMonth, storedYear);
      rtc.adjust(DateTime(storedYear, storedMonth, storedDay, storedHour, storedMinute, storedSecond));
      adjustTime(1);                                     // 1pps signal = start of next second
      lastMillis = millis();
      hasTimeBeenSet = true;
      syncReady = false;
      lastMinute = storedMinute;

      Serial.println("Synced!");
    }
  }
  pps = 0;
}

void loop() {
  if (!hasTimeBeenSet) { // if we have not yet set the time
    Serial.print("Num satelites: "); Serial.println(gps.satellites());
    //Serial.println("Attempting top set time");
    while (Serial3.available()) {
      if (gps.encode(Serial3.read())) { // process gps messages
        // new data...let's crack the date/time
        gps.crack_datetime(&storedYear, &storedMonth, &storedDay, &storedHour, &storedMinute, &storedSecond, &storedHundredths, &storedAge);
        Serial.print("Cracked a new time! Second is "); Serial.print(static_cast<int>(storedSecond));
        Serial.print(", Age is "); Serial.println(static_cast<int>(storedAge));
        if (storedAge < 1000) {
          // it's good data (not old)...so, let's use it
          syncReady = true;
        } else {Serial.println("Could not set time, Data too old");}
      } else {Serial.println("Could not set time, no data to read");}
      syncCheck(); // check for a sync after attempting to crack a new data stream TODO: we may not need to check here if the last crack failed.
    }
    syncCheck(); // check for a sync after we've finished running through all available GPS data TODO: we may not need to check here if the last crack failed.
  }

  // check if its time to check for the dip settings TODO: move this to a scheduled task, see branch task-scheduler
  if (dipcheck++ > 80000){ // Check if 80000 cycles have passed since last updating the dips

    // update timezone
    _timeZone = 0;
    for (byte i = 0; i<sizeof TimeZoneInputs/sizeof TimeZoneInputs[0]; i++) {
      byte value = digitalRead(TimeZoneInputs[i]); // read byte (0001, 0000)
      _timeZone = _timeZone + (value << i); // shif byte to its correct magnitude
    }
    timeZone = _timeZone;
    Serial.print("Timezone is: "); Serial.println(timeZone);

    // update clock format
    if (digitalRead(ClockFormatInput)) {
      clockFormat = 24;
    } else {
      clockFormat = 12;
    }

    dipcheck = 0; // Return dipcheck to 0
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
