/**
 * @file        getData.cpp
 * @author      NGUYEN QUOC VUONG
 * @note        This project is part of ESP32-AI series.
 * @copyright   Copyright (c) 2026 Nguyen Quoc Vuong. All rights reserved.
 **/

#include "getData.h"

//Define I2C PINs For ESP32
#define I2C_MASTER_SCL_IO           14
#define I2C_MASTER_SDA_IO           13
//Indicate That We Use I2C Number 0 (There Are 2 I2Cs In ESP32: I2C_NUM_0 And I2C_NUM_1)
#define I2C_MASTER_NUM              I2C_NUM_0 
//I2C Clock Frequency (Standard: 400kHz)
#define I2C_MASTER_FREQ_HZ          400000 
//Default Address Of MPU6050
#define MPU6050_ADDR                0x68


esp_err_t MPU6050::begin() {
    // Reset MPU6050
    writeRegister(0x6B, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Configure ESP32 I2C Parameters
    i2c_config_t conf = { 
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ
        },
        .clk_flags = 0
    };

    // Apply Configuration To I2C Peripheral 
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    // Install I2C Driver For Master
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) return err;

    // Choose DLPF with mode number 5: ~10Hz
    writeRegister(0x1A, 0x05);

    // Set Accelerometer's Full-Scale Range to ±4g (ASF_SEL Mode 1)
    writeRegister(0x1C, 0x08);

    // Select X-axis Gyroscope As Clock Source And Wake Up
    return writeRegister(0x6B, 0x01);
}

esp_err_t MPU6050::readAccel(float &ax, float &ay, float &az) {
    // 6 bytes of data (2 bytes/ 1 axis)
    uint8_t data[6];
    // Starting Register Adress
    uint8_t reg = 0x3B;
    // Get The Data
    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        ax = (float)((int16_t)(data[0] << 8 | data[1])) / 8192.0f;  // Divide By 8192.0 Because It's A Sensitivity Scale Factor For ±4g Range
        ay = (float)((int16_t)(data[2] << 8 | data[3])) / 8192.0f;  // Divide By 8192.0 Because It's A Sensitivity Scale Factor For ±4g Range
        az = (float)((int16_t)(data[4] << 8 | data[5])) / 8192.0f;  // Divide By 8192.0 Because It's A Sensitivity Scale Factor For ±4g Range
    }
    return err;
}

esp_err_t MPU6050::readGyro(float &gx, float &gy, float &gz) {
    // 6 bytes of data (2 bytes/ 1 axis)
    uint8_t data[6];
    // Starting Register Adress
    uint8_t reg = 0x43;
    // Get the data
    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        gx = (float)((int16_t)(data[0] << 8 | data[1])) / 131.0f;   // Divide By 131.0 Because It's A Sensitivity Scale Factor For Default Range
        gy = (float)((int16_t)(data[2] << 8 | data[3])) / 131.0f;   // Divide By 131.0 Because It's A Sensitivity Scale Factor For Default Range
        gz = (float)((int16_t)(data[4] << 8 | data[5])) / 131.0f;   // Divide By 131.0 Because It's A Sensitivity Scale Factor For Default Range
    }
    return err;
}

esp_err_t MPU6050::writeRegister(uint8_t reg_addr, uint8_t data) {
    // Buffer (Address + Data) 
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
}