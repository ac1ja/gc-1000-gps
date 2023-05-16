#include <Arduino.h>
#include <ArduinoLog.h>
#include <Arduino_FreeRTOS.h>

char cmdIn[24];
byte index = 0;
char EOT = '\n';
bool echo = true;

void TaskSerialInstruction(void *pvParameters)
{
    for (;;)
    {
        if (Serial.available() > 0)
        {
            // Serial is available
            char inByte = Serial.read();
            if (index < 24)
            {
                // Record this byte in our cmdIn
                cmdIn[index] = inByte;
                cmdIn[index + 1] = '\0'; // Null terminate
                index++;

                // Echo back if enabled
                if (echo)
                {
                    Serial.print(inByte);
                }
            }

            // Check if we got a whole command
            if (inByte == EOT)
            {
                if (strcmp(cmdIn, "version?\r\n") == 0)
                {
                    Log.noticeln(VER);
                }
                else if (strcmp(cmdIn, "sat?\r\n") == 0)
                {
                    Log.noticeln("Num satellites: %d", gps.satellites());
                }
                else
                {
                    Serial.print(cmdIn);
                    Serial.println("?");
                }

                cmdIn[0] = '\0';
                index = 0;
            }
        }
        else
        {
            vTaskDelay(8);
        }
    }
}