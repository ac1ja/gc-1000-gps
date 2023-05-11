/**
 * @file gps.h
 * @author Joe
 * @brief Methods and tasks related to the gps live here.
 */

#include "Arduino.h"
#include <Arduino_FreeRTOS.h>

// void TaskReadSwitches(void *pvParameters);

void TaskGPSCommunicate(void *pvParameters)
{
    // Suspend/delay for 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
