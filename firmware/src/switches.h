/**
 * @file switches.h
 * @author Joe
 * @brief Methods and tasks related to the dip switches live here.
 */

#include "Arduino.h"
#include <Arduino_FreeRTOS.h>

// void TaskReadSwitches(void *pvParameters);

void TaskReadSwitches(void *pvParameters)
{
    // Suspend/delay for 100ms
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
