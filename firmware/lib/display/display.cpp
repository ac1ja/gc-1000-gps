/**
 * @file display.cpp
 * @author Joe (joe.sedutto@silvertech.com)
 * @brief Methods and functions for the GC-1000-GPS Display
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <Arduino.h>

#include "display.h"
#include "buildData.h"
#include "shiftMSBOut.h"

uint8_t Display::buildTimeData(uint8_t digit, uint8_t position) { return (digit + (position << 4)); };

uint8_t Display::buildStatusData(bool AMLED, bool PMLED, bool hiSpecLED, bool captureLED, bool mhz15LED, bool mhz10LED, bool mhz5LED, bool dataLED)
{
    uint8_t statusByte = 0;

    statusByte = ((AMLED == true) ? 1 : 0);
    statusByte += ((PMLED == true) ? 2 : 0);
    statusByte += ((hiSpecLED == true) ? 4 : 0);
    statusByte += ((captureLED == true) ? 8 : 0);
    statusByte += ((mhz15LED == true) ? 16 : 0);
    statusByte += ((mhz10LED == true) ? 32 : 0);
    statusByte += ((mhz5LED == true) ? 64 : 0);
    statusByte += ((dataLED == true) ? 128 : 0);

    return (statusByte);
}

Display::Display(uint8_t _segEnablePin, uint8_t _latchPin, uint8_t _dataPin, uint8_t _clockPin) : segEnablePin(_segEnablePin), latchPin(_latchPin), dataPin(_dataPin), clockPin(_clockPin)
{
    pinMode(segEnablePin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
};

void Display::updateBoard()
{
    dispData += (buildStatusData(AM, PM, highSpecLED, captureLED, mhz15, mhz10, mhz5, dataLED) << 8);

    if (++currentSegment > 6) // add 1 to pos and check if it's larger than 6
        currentSegment = 0;   // clamp value

    digitalWrite(segEnablePin, 0);
    digitalWrite(latchPin, 0);
    shiftMSBOut(dataPin, clockPin, dispData);
    digitalWrite(latchPin, 1);
    digitalWrite(segEnablePin, 1);
};

void Display::setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t tenths)
{
    switch (currentSegment)
    { // switch on digit location
    case 0:
        dispData = buildTimeData(hour / 10, 0);
        break;
    case 1:
        dispData = buildTimeData(hour % 10, 1);
        break;
    case 2:
        dispData = buildTimeData(minute / 10, 2);
        break;
    case 3:
        dispData = buildTimeData(minute % 10, 3);
        break;
    case 4:
        dispData = buildTimeData(second / 10, 4);
        break;
    case 5:
        dispData = buildTimeData(second % 10, 5);
        break;
    case 6:
        dispData = buildTimeData(tenths, 6);
    }
};
void Display::setMeridan(bool _AM, bool _PM)
{
    AM = _AM;
    PM = _PM;
};
void Display::setHighSpec(bool highSpec) { highSpecLED = highSpec; };
void Display::setCapture(bool capture) { captureLED = capture; };
void Display::setData(bool data) { dataLED = data; };
void Display::setDrift(Drift drift)
{
    mhz5 = bitRead(drift, 0);
    mhz10 = bitRead(drift, 1);
    mhz15 = bitRead(drift, 2);
};
