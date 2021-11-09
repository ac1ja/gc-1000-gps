#include "buildData.h"
#include "timezones.h"


byte pos = 0;
bool AM, PM;

byte localHour, localMinute, localSecond, localTens;
byte dataLED, captureLED, highSpecLED = false;

bool mhz5, mhz10, mhz15;

//time_t local = myTZ.toLocal(now()); // This bogs the system down if run too fast!

void updateBoard(void) {
  // read the status of comm pins
  dataLED = !digitalRead(debugSerialCheck); // if there is data on the serial line
  captureLED = !digitalRead(gpsSerialCheck); // if the gps is being read from
  highSpecLED = hasTimeBeenSet; // if the time has been locked in/synced to the rtc

  localHour =  getUTCOffsetHours(hour());
  localMinute = getUTCOffsetMinutes(minute());
  localSecond = second();
  localTens = (((millis() - lastMillis) / 100) % 10);

  AM = getAM(hour());
  PM = !AM;

  // Mhz lights.
  mhz5 = true;
  if (storedAge == TinyGPS::GPS_INVALID_AGE || storedAge == 0) {
    mhz10 = false;
  } else {
    mhz10 = true;
  }
  mhz15 = storedAge <= 55 && storedAge != 0;

  //if (localTens < 1)
  //  dataLED = true;

  switch (pos) { // switch on digit location
    case 0:
      dataOut = buildTimeData(localHour / 10, 0);
      break;
    case 1:
      dataOut = buildTimeData(localHour % 10, 1);
      break;
    case 2:
      dataOut = buildTimeData(localMinute / 10, 2);
      break;
    case 3:
      dataOut = buildTimeData(localMinute % 10, 3);
      break;
    case 4:
      dataOut = buildTimeData(localSecond / 10, 4);
      break;
    case 5:
      dataOut = buildTimeData(localSecond % 10, 5);
      break;
    case 6:
      dataOut = buildTimeData(localTens, 6);
  }

  dataOut += (buildStatusData(AM, PM, highSpecLED, captureLED, mhz15, mhz10, mhz5, dataLED) << 8);

  if(++pos > 6) // add 1 to pos and check if it's larger than 6
    pos = 0; // clamp value

  digitalWrite(SEGMENT_ENABLE_PIN, 0);
  digitalWrite(LATCH_PIN, 0);
  shiftOut(DATA_PIN, CLOCK_PIN, dataOut);
  digitalWrite(LATCH_PIN, 1);
  digitalWrite(SEGMENT_ENABLE_PIN, 1);
}