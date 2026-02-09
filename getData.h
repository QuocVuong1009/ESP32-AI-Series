/**
 * @file        getData.h
 * @author      NGUYEN QUOC VUONG
 * @note        This project is part of ESP32-AI series.
 * @copyright   Copyright (c) 2026 Nguyen Quoc Vuong. All rights reserved.
 **/

#pragma once

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class MPU6050 {
public:
    //Initialize the MPU6050 sensor
    esp_err_t begin() ;

    //Read Accelerometer Data
    esp_err_t readAccel(float &ax, float &ay, float &az) ;

    //Read Gyroscope Data
    esp_err_t readGyro(float &gx, float &gy, float &gz) ;

private:
    //Helper Method To Write Data To A Specific Register
    esp_err_t writeRegister(uint8_t reg_addr, uint8_t data) ;
};