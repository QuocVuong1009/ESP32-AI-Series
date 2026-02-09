/**
 * @file        mainAI.cpp
 * @author      NGUYEN QUOC VUONG
 * @note        This project is part of ESP32-AI series.
 * @copyright   Copyright (c) 2026 Nguyen Quoc Vuong. All rights reserved.
 **/

#include <cmath>
#include "getData.h"
#include "model_data.h" 

// Libraries For TensorFlow Lite Micro On ESP32 
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Configuration Macros For Gesture Prediction
#define SAMPLE_COUNT_PER_GESTURE 100     // Total Samples Per One Gesture
#define SAMPLE_DELAY_MS          20      // Sampling Speed (50Hz)
#define ACCEL_THRESHOLD          1.25f   // Acceleration Trigger Threshold
#define CONFIDENCE_THRESHOLD     0.8f    // Minimun Confidencehere Is Not  Score For A Valid Gesture Prediction
#define NA_PREDICTION            -1      // No valid prediction has been performed
#define VALID_NUMBER_PREDICTION  4       // Valid Number Of Predictions Threshold

// Mean And Standard Deviation For Normalization (Z-Score)
const float MEAN[] = { 0.9115, -0.0792, -0.2418, 2.3791, -10.2013, -1.6355 };
const float STD[]  = { 0.2704, 0.2168, 0.3940, 61.2469, 98.9243, 28.8040 };

// Global Variables For TFLITE
namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    
    // Tensor Arena Size: 32KB Allocated For TFLite Operations
    constexpr int kTensorArenaSize = 32 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
}

// Initialization Function For Setting Up AI Model
void setupAI() {
    // Setup Error Reporter
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Load Model
    model = tflite::GetModel(NGUYEN_QUOC_VUONG); 
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Model version mismatch!\n");
        return;
    }

    // Declare Ops
    static tflite::MicroMutableOpResolver<5> resolver;
    resolver.AddReshape();     
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddSoftmax();    
    
    // Initialize Interpreter (model, resolver, arena, arena_size, resource_vars, profiler)
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, nullptr, nullptr);
    interpreter = &static_interpreter;

    // Allocate Interpreter
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("AllocateTensors failed!\n");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    printf("AI Setup DONE! Input size: %d\n", input->bytes);
}

//Main Function For Running AI Model
extern "C" void app_main(void) {
    // Initialize The MPU6050 Sensor
    MPU6050 sensor;
    if (sensor.begin() != ESP_OK) return;
    
    // Initialize AI Model
    setupAI();

    // Parameters For Reading Data
    float ax, ay, az, gx, gy, gz;
    int samplesTaken = 0;
    bool isRecording = false;
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

                // Normalize Data And Feed Into Model's Input Tensor
                int base_idx = samplesTaken * 6;
                input->data.f[base_idx + 0] = (ax - MEAN[0]) / STD[0];
                input->data.f[base_idx + 1] = (ay - MEAN[1]) / STD[1];
                input->data.f[base_idx + 2] = (az - MEAN[2]) / STD[2];
                input->data.f[base_idx + 3] = (gx - MEAN[3]) / STD[3];
                input->data.f[base_idx + 4] = (gy - MEAN[4]) / STD[4];
                input->data.f[base_idx + 5] = (gz - MEAN[5]) / STD[5];
                samplesTaken++;

                // Stop Recording After Reaching The Sample Limit
                if (samplesTaken >= SAMPLE_COUNT_PER_GESTURE) {
                    printf(">>> INFERENCE...\n");
                    
                    //Parameters For Predicting
                    int myResult = NA_PREDICTION;   // The Number That Has Been Predicted
                    int resultArr[10] = {};         // An Array That Stores Total Prediction Of Each Number 
                    int myMax = 0;                  // Total Prediction Of The Number
                    for (int i = 0; i < 10; ++i) {
                        // Run AI Model On The Data
                        TfLiteStatus invoke_status = interpreter->Invoke();
                        
                        if (invoke_status != kTfLiteOk) {
                            printf("Invoke failed!\n");
                            break;
                        } else {
                            float max_score = 0;            // The Score Of The Highest Prediction
                            int max_index = NA_PREDICTION;  // The Number That Has Been Predicted
                            // Find Label That Has The Highest Result
                            for (int j = 0; j < 10; ++j) {
                                float score = output->data.f[j];
                                if (score > max_score && score > CONFIDENCE_THRESHOLD) {
                                    max_score = score;
                                    max_index = j;
                                }
                            }
                            
                            // Predicted A Number
                            if (max_index != NA_PREDICTION) {
                                if (++resultArr[max_index] > myMax) {
                                    myMax = resultArr[max_index];
                                    myResult = max_index;
                                }
                                printf("Result In The %d Time, Number %d, (Confidence: %.1f%%)\n", i+1, myResult, max_score * 100);
                            }
                            else {
                                printf("CAN'T Be Predict I The %d Time\n", i+1);
                            }
                        }
                    }

                    // Outputing The Result
                    printf("--------------------------------\n");
                    if (myResult != NA_PREDICTION && myMax >= VALID_NUMBER_PREDICTION) {
                        printf("The Final Result: Number %d \n", myResult);
                    } else {
                        printf("CAN'T Be Predict \n");
                    }
                    printf("--------------------------------\n");

                    // Cooldown: Wait For The Sensor To Stabilize Before The Next Gesture
                    isRecording = false;
                    printf("Waiting for cooldown...\n");
                    while (true) {
                        sensor.readAccel(ax, ay, az);
                        float current_mag = sqrt(ax*ax + ay*ay + az*az);
                        // Lower The Threshold To Prepare For The Next Gesture
                        if (current_mag < 1.15f && current_mag > 0.85f) { 
                            vTaskDelay(pdMS_TO_TICKS(500)); 
                            break; 
                        }
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }
                    printf("--> READY!\n");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_DELAY_MS));
    }
}