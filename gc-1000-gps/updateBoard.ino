#include "buildData.h"
#include "timezones.h"


byte pos = 0;
bool AM, PM;

byte localHour, localMinute, localSecond, localTens;
byte dataLED = false;

bool mhz5, mhz10, mhz15;

void updateBoard(void) {
  dataLED = false;

  localHour =  getUTCOffsetHours(hour());
  localMinute = getUTCOffsetMinutes(minute());
  localSecond = second();
  localTens = (((millis() - lastMillis) / 100) % 10);

  AM = getAM(hour());
  PM = !AM;
  
  if (localSecond < 20) { // temporary use of the LEDs
    mhz5 = true;
    mhz10 = false;
    mhz15 = false;
  } else { 
    if(localSecond < 40) {
      mhz5 = false;
      mhz10 = true;
      mhz15 = false;
    } else {
      mhz5 = false;
      mhz10 = false;
      mhz15 = true;
    }
  }

  if (localTens < 1)
    dataLED = true;

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

  dataOut += (buildStatusData(AM, PM, hasTimeBeenSet, !hasTimeBeenSet, mhz15, mhz10, mhz5, dataLED) << 8);

  if(++pos > 6) // add 1 to pos and check if it's larger than 6
    pos = 0; // clamp value

  digitalWrite(SEGMENT_ENABLE_PIN, 0);
  digitalWrite(LATCH_PIN, 0);
  shiftOut(DATA_PIN, CLOCK_PIN, dataOut);
  digitalWrite(LATCH_PIN, 1);
  digitalWrite(SEGMENT_ENABLE_PIN, 1);
}