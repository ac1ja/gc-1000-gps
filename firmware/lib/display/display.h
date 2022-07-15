/**
 * @file display.h
 * @author Joe (joe.sedutto@silvertech.com)
 * @brief Data and Prototypes for the GC-1000-GPS Display
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <Arduino.h>

/**
 * @brief Display class used to interact with the standard gc-1000-gps display
 *
 * @author Joe (joe.sedutto@silvertech.com)
 *
 */
class Display
{
private:
    // Pins
    const uint8_t segEnablePin;
    const uint8_t latchPin;
    const uint8_t dataPin;
    const uint8_t clockPin;

    // RTC Drift
    uint8_t lastDrift = NONE;

    // Segments
    const uint8_t displaySegments = 6;
    uint8_t currentSegment = 0;
    uint16_t dispData;

    // Lamps
    bool AM, PM, highSpecLED, captureLED, mhz15, mhz10, mhz5, dataLED;

    uint8_t buildTimeData(uint8_t digit, uint8_t position);
    uint8_t buildStatusData(bool AMLED, bool PMLED, bool hiSpecLED, bool captureLED, bool mhz15LED, bool mhz10LED, bool mhz5LED, bool dataLED);

public:
    // RTC Drift
    enum Drift
    {
        SLOW,
        NONE,
        FAST,
    };

    Display(uint8_t _segEnablePin, uint8_t _latchPin, uint8_t _dataPin, uint8_t _clockPin);

    void updateBoard();

    void setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t tenths);
    void setMeridan(bool AM, bool PM);
    void setHighSpec(bool highSpec);
    void setCapture(bool capture);
    void setDrift(Drift drift);
};
