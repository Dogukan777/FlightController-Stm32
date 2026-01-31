#include "gyroscope.h"
#include <string.h>
#include <stdio.h>

// Senin OLED fonksiyonun main.c’de var diye varsayıyorum:
extern void displayTwoLines(const char *top, const char *bottom);
extern void displayScreen(const char *str);

// MPU6050 I2C address
#define MPU6050_ADDR_7BIT   0x68
#define MPU6050_ADDR        (MPU6050_ADDR_7BIT << 1)

// Registers
#define REG_PWR_MGMT_1      0x6B
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_ACCEL_XOUT_H    0x3B
#define REG_WHO_AM_I        0x75

// Helpers
static uint8_t write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
  return (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                           &val, 1, 100) == HAL_OK);
}

static uint8_t read_regs(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                          buf, len, 200) == HAL_OK);
}

uint8_t GYRO_Init(I2C_HandleTypeDef *hi2c)
{
  // Device ready?
  if (HAL_I2C_IsDeviceReady(hi2c, MPU6050_ADDR, 3, 200) != HAL_OK) {
    return 0;
  }

  // WHO_AM_I (opsiyonel kontrol)
  uint8_t who = 0;
  if (read_regs(hi2c, REG_WHO_AM_I, &who, 1)) {
    // Normalde 0x68 beklenir (bazı varyantlarda farklı olabilir)
    // İstersen debug için ekrana bas:
    // char msg[20]; snprintf(msg, sizeof(msg), "WHO:%02X", who); displayScreen(msg);
  }

  // Wake up (sleep=0)
  if (!write_reg(hi2c, REG_PWR_MGMT_1, 0x00)) return 0;
  HAL_Delay(50);

  // Basic config
  // Sample rate divider
  write_reg(hi2c, REG_SMPLRT_DIV, 0x07);   // ~1kHz/(1+7)=125Hz (DLPF'e göre değişir)
  // DLPF config
  write_reg(hi2c, REG_CONFIG, 0x06);
  // Gyro full-scale: 0x00 => ±250 dps
  write_reg(hi2c, REG_GYRO_CONFIG, 0x00);
  // Accel full-scale: 0x00 => ±2g
  write_reg(hi2c, REG_ACCEL_CONFIG, 0x00);

  return 1;
}

uint8_t GYRO_ReadRaw(I2C_HandleTypeDef *hi2c, MPU6050_Raw *out)
{
  uint8_t b[14];
  if (!read_regs(hi2c, REG_ACCEL_XOUT_H, b, 14)) return 0;

  out->ax       = (int16_t)((b[0]  << 8) | b[1]);
  out->ay       = (int16_t)((b[2]  << 8) | b[3]);
  out->az       = (int16_t)((b[4]  << 8) | b[5]);
  out->temp_raw = (int16_t)((b[6]  << 8) | b[7]);
  out->gx       = (int16_t)((b[8]  << 8) | b[9]);
  out->gy       = (int16_t)((b[10] << 8) | b[11]);
  out->gz       = (int16_t)((b[12] << 8) | b[13]);

  return 1;
}

void GYRO_Convert(const MPU6050_Raw *raw, MPU6050_Scaled *out)
{
  // Config: accel ±2g => 16384 LSB/g
  // gyro  ±250dps => 131 LSB/(deg/s)
  out->ax_g = (float)raw->ax / 16384.0f;
  out->ay_g = (float)raw->ay / 16384.0f;
  out->az_g = (float)raw->az / 16384.0f;

  out->gx_dps = (float)raw->gx / 131.0f;
  out->gy_dps = (float)raw->gy / 131.0f;
  out->gz_dps = (float)raw->gz / 131.0f;

  out->temp_c = ((float)raw->temp_raw / 340.0f) + 36.53f;
}

void GYRO_PrintToOLED(const MPU6050_Scaled *v)
{
  static uint8_t page = 0;
  char top[32];
  char bottom[32];

  switch (page)
  {
    case 0: // Accel X-Y
      snprintf(top, sizeof(top), "AX:%0.2fg", v->ax_g);
      snprintf(bottom, sizeof(bottom), "AY:%0.2fg", v->ay_g);
      break;

    case 1: // Accel Z + Temp
      snprintf(top, sizeof(top), "AZ:%0.2fg", v->az_g);
      snprintf(bottom, sizeof(bottom), "T:%0.1fC", v->temp_c);
      break;

    case 2: // Gyro X-Y
      snprintf(top, sizeof(top), "GX:%0.1fdps", v->gx_dps);
      snprintf(bottom, sizeof(bottom), "GY:%0.1fdps", v->gy_dps);
      break;

    default: // Gyro Z
      snprintf(top, sizeof(top), "GZ:%0.1fdps", v->gz_dps);
      snprintf(bottom, sizeof(bottom), ""); // boş satır
      break;
  }

  displayTwoLines(top, bottom);

  page++;
  if (page > 3) page = 0;
}
void GYRO_PrintXYZRawToOLED(const MPU6050_Raw *r)
{
  static uint8_t page = 0;
  static char top[20];
  static char bottom[20];

  // 16-17 karakteri geçmeyelim
  switch (page)
  {
    case 0:
      snprintf(top,    sizeof(top),    "AX:%d", (int)r->ax);
      snprintf(bottom, sizeof(bottom), "AY:%d", (int)r->ay);
      break;

    case 1:
      snprintf(top,    sizeof(top),    "AZ:%d", (int)r->az);
      snprintf(bottom, sizeof(bottom), "GX:%d", (int)r->gx);
      break;

    case 2:
      snprintf(top,    sizeof(top),    "GY:%d", (int)r->gy);
      snprintf(bottom, sizeof(bottom), "GZ:%d", (int)r->gz);
      break;

    default:
      snprintf(top,    sizeof(top),    "Traw:%d", (int)r->temp_raw);
      snprintf(bottom, sizeof(bottom), "");
      break;
  }

  displayTwoLines(top, bottom);
  page = (page + 1) & 3;   // 0..3
}





