#ifndef GYROSCOPE_H
#define GYROSCOPE_H

#include "main.h"
#include <stdint.h>
#include <math.h>

typedef struct {
    int16_t ax, ay, az;
    int16_t temp_raw;
    int16_t gx, gy, gz;
} MPU9250_Raw;

typedef struct {
    int16_t mx, my, mz;
} AK8963_Raw;

typedef struct {
    float roll;
    float pitch;
    float yaw;
} Attitude_t;


#define MPU6050_ADDR_1   (0x68 << 1) // AD0 -> GND
#define MPU6050_ADDR_2   (0x69 << 1) // AD0 -> 3.3V

uint8_t MPU9250_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU9250_ReadRaw(I2C_HandleTypeDef *hi2c, MPU9250_Raw *out);
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU6050_Read6Axis(I2C_HandleTypeDef *hi2c, int16_t *accel, int16_t *gyro);
uint8_t AK8963_Init(I2C_HandleTypeDef *hi2c);
uint8_t AK8963_ReadRaw(I2C_HandleTypeDef *hi2c, AK8963_Raw *out);

void ATTITUDE_FromAccelMag(const MPU9250_Raw *imu, const AK8963_Raw *mag, Attitude_t *att);
uint8_t MPU9250_GetAttitude(I2C_HandleTypeDef *hi2c, Attitude_t *att);

#endif
