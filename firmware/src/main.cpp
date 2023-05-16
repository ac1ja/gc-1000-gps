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
#include <Arduino_FreeRTOS.h>
#include <RTClib.h>          // https://github.com/adafruit/RTClib
#include <TimeLib.h>         // https://github.com/PaulStoffregen/Time
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include <TimerOne.h>        // https://github.com/PaulStoffregen/TimerOne
// #include <avr/wdt.h>
#include <ArduinoLog.h>

// Our libs
#include "display.h"

// Our headers
#include "boardConfig.h"
#include "buildData.h"
#include "timezones.h"

// hardware objects
RTC_DS3231 rtc;

// Display
Display display(SEGMENT_ENABLE_PIN, LATCH_PIN, DATA_PIN, CLOCK_PIN);

#include "switches.h"
#include "gps.h"
#include "serial.h"

// display lights
const byte debugSerialCheck = 1; // debug activity pin
const byte gpsSerialCheck = 15;  // gps activity pin

time_t prevDisplay = 0; // when the digital clock was displayed

bool AM, PM;

byte localHour, localMinute, localSecond, localTens;

void updateBoard(void)
{
  // read the status of comm pins
  display.setData(!digitalRead(debugSerialCheck));                                         // if there is data on the serial line
  display.setCapture(displayPPS);                                                          // if the gps is being read from
  display.setHighSpec(lastTimeSync == 0 ? false : millis() - lastTimeSync < hiSpecMaxAge); // if the time has been locked in/synced to the rtc

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

  lastTimeSync = 0;
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

  // Load prev RTC time
  setTime(rtc.now().unixtime());

  // Configure watchdog
  // wdt_enable(WDTO_2S); // In the future, consider using a different internal timer to keep track of the WDT

  // Initialize interrupts
  Timer1.initialize(3000);             // Cycle every 3000μs
  Timer1.attachInterrupt(updateBoard); // Attach an interrupt to callback updateBoard()

  // Configure Tasks
  xTaskCreate(
      TaskReadSwitches, "ReadSwitches",
      128,     // Stack size
      NULL, 1, // Priority
      NULL);

  xTaskCreate(
      TaskBlinkOnPPS, "BlinkOnPPS",
      128,     // Stack size
      NULL, 1, // Priority
      NULL);

  xTaskCreate(
      TaskSerialInstruction, "SerialInstruction",
      128,     // Stack size
      NULL, 2, // Priority
      NULL);

  xTaskCreate(
      TaskGPSCommunicate, "GPSCommunicate",
      4096,    // Stack size
      NULL, 2, // Priority
      NULL);
}

void loop()
{
  // Trigger Watchdog TODO: Move me to an idle task!
  // wdt_reset();
}
