#ifndef GYROSCOPE_H
#define GYROSCOPE_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "SH1106.h"
#include "fonts.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  int16_t temp_raw;
} MPU6050_Raw;

typedef struct {
  float ax_g, ay_g, az_g;      // g
  float gx_dps, gy_dps, gz_dps; // deg/s
  float temp_c;               // Celsius
} MPU6050_Scaled;

/**
 * @brief  MPU6050 init (wake up + basic config)
 * @param  hi2c : I2C handle (senin durumda &hi2c2)
 * @return 1 OK, 0 FAIL
 */
uint8_t GYRO_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief  Read raw accel/gyro/temp
 */
uint8_t GYRO_ReadRaw(I2C_HandleTypeDef *hi2c, MPU6050_Raw *out);

/**
 * @brief  Convert raw to scaled units
 */
void GYRO_Convert(const MPU6050_Raw *raw, MPU6050_Scaled *out);

/**
 * @brief  Quick screen print helper (2 satır)
 *         displayTwoLines fonksiyonunu kullanır
 */
void GYRO_PrintToOLED(const MPU6050_Scaled *v);
void GYRO_PrintXYZRawToOLED(const MPU6050_Raw *r);


#ifdef __cplusplus
}
#endif

#endif
