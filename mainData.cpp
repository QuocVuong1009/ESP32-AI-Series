/**
 * @file        mainData.cpp
 * @author      NGUYEN QUOC VUONG
 * @note        This project is part of ESP32-AI series.
 * @copyright   Copyright (c) 2026 Nguyen Quoc Vuong. All rights reserved.
 **/

#include <math.h>
#include "getData.h"

// Configuration Macros For Sampling Data
#define SAMPLE_COUNT_PER_GESTURE    100      // Total Samples Per One Gesture
#define SAMPLE_DELAY_MS             20       // Sampling Speed (50Hz)
#define ACCEL_THRESHOLD             1.25f    // Acceleration Trigger Threshold

extern "C" void app_main(void) {
    // Initialize The MPU6050 Sensor
    MPU6050 sensor;
    if (sensor.begin() != ESP_OK) return;

    // Parameters For Sampling Data
    float ax, ay, az, gx, gy, gz;
    bool isRecording = false;
    int samplesTaken = 0;
    printf("\nThe System Is Ready, Please Take An Action\n");

    while (1) {
        // Reading MPU6050's Data
        if (sensor.readAccel(ax, ay, az) == ESP_OK && sensor.readGyro(gx, gy, gz) == ESP_OK) {
            
            // Calculate Acceleration Magnitude
            float magnitude = sqrt(ax*ax + ay*ay + az*az);

            // Check The Condition To Start Sampling (Trigger)
            if (!isRecording && magnitude > ACCEL_THRESHOLD) {
                isRecording = true;
                samplesTaken = 0;
                printf("\n>>> START RECORDING...\n");
            }

            if (isRecording) {
                printf("%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n", ax, ay, az, gx, gy, gz);
                samplesTaken++;

                // Stop Recording After Reaching The Sample Limit
                if (samplesTaken >= SAMPLE_COUNT_PER_GESTURE) {
                    isRecording = false;
                    printf("\n------------- END ----------------\n");
                    printf("----------------------------------\n");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_DELAY_MS));
    }
}