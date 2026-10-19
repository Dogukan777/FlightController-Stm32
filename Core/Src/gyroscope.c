#include "gyroscope.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define MAG_OFFSET_X  20.0f
#define MAG_OFFSET_Y  -35.0f
#define MAG_OFFSET_Z  15.0f
#define ALPHA 0.98
#define DT 0.02

// MPU9250 Adres Tanımlamaları
#define MPU9250_ADDR_7BIT   0x68
#define MPU9250_ADDR        (MPU9250_ADDR_7BIT << 1)

// MPU6050 Adres Tanımlamaları (Önceki projenden gelen kararlı dinamik yapı)
#define MPU6050_ADDR_A_7BIT   0x68
#define MPU6050_ADDR_B_7BIT   0x69

static uint8_t dynamic_mpu6050_addr = (0x68 << 1);

// Ortak Register Tanımlamaları
#define REG_PWR_MGMT_1      0x6B
#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C
#define REG_ACCEL_XOUT_H    0x3B
#define REG_WHO_AM_I        0x75
#define REG_INT_PIN_CFG     0x37
#define REG_USER_CTRL       0x6A

// AK8963 Manyetometre Tanımlamaları
#define AK8963_ADDR_7BIT    0x0C
#define AK8963_ADDR         (AK8963_ADDR_7BIT << 1)

#define AK8963_WIA          0x00
#define AK8963_ST1          0x02
#define AK8963_XOUT_L       0x03
#define AK8963_CNTL1        0x0A
#define AK8963_CNTL2        0x0B
#define AK8963_ASAX         0x10

// Extern OLED Fonksiyonları
extern void displayTwoLines(const char *top, const char *bottom);
extern void displayScreen(const char *str);

// ====================================================================
// ALT SEVİYE I2C YARDIMCI FONKSİYONLARI (Alt Yapı Bozulmadı)
// ====================================================================

static uint8_t mpu_write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
    return (HAL_I2C_Mem_Write(hi2c, MPU9250_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                              &val, 1, 100) == HAL_OK);
}

static uint8_t mpu_read_regs(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(hi2c, MPU9250_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                             buf, len, 100) == HAL_OK);
}

static uint8_t ak_write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
    return (HAL_I2C_Mem_Write(hi2c, AK8963_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                              &val, 1, 100) == HAL_OK);
}

static uint8_t ak_read_regs(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(hi2c, AK8963_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                             buf, len, 100) == HAL_OK);
}

// ====================================================================
// MPU9250 SENSÖR FONKSİYONLARI (Kesinlikle Dokunulmadı)
// ====================================================================

uint8_t MPU9250_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t who = 0;

    if (HAL_I2C_IsDeviceReady(hi2c, MPU9250_ADDR, 3, 100) != HAL_OK)
        return 0;

    if (!mpu_read_regs(hi2c, REG_WHO_AM_I, &who, 1))
        return 0;

    // sleep kapat
    if (!mpu_write_reg(hi2c, REG_PWR_MGMT_1, 0x00)) return 0;
    HAL_Delay(50);

    // internal i2c master kapalı
    if (!mpu_write_reg(hi2c, REG_USER_CTRL, 0x00)) return 0;

    // bypass enable -> AK8963 doğrudan erişilsin
    if (!mpu_write_reg(hi2c, REG_INT_PIN_CFG, 0x02)) return 0;
    HAL_Delay(10);

    if (!mpu_write_reg(hi2c, REG_SMPLRT_DIV, 0x07)) return 0;
    if (!mpu_write_reg(hi2c, REG_CONFIG, 0x06)) return 0;
    if (!mpu_write_reg(hi2c, REG_GYRO_CONFIG, 0x00)) return 0;   // ±250 dps
    if (!mpu_write_reg(hi2c, REG_ACCEL_CONFIG, 0x00)) return 0;  // ±2g

    return 1;
}

uint8_t MPU9250_ReadRaw(I2C_HandleTypeDef *hi2c, MPU9250_Raw *out)
{
    uint8_t b[14];

    if (!mpu_read_regs(hi2c, REG_ACCEL_XOUT_H, b, 14))
        return 0;

    out->ax = (int16_t)((b[0]  << 8) | b[1]);
    out->ay = (int16_t)((b[2]  << 8) | b[3]);
    out->az = (int16_t)((b[4]  << 8) | b[5]);
    out->temp_raw = (int16_t)((b[6]  << 8) | b[7]);
    out->gx = (int16_t)((b[8]  << 8) | b[9]);
    out->gy = (int16_t)((b[10] << 8) | b[11]);
    out->gz = (int16_t)((b[12] << 8) | b[13]);

    return 1;
}

// ====================================================================
// MPU6050 SENSÖR FONKSİYONLARI (Entegre Edilen Kararlı Yapı)
// ====================================================================

uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t who = 0;
    uint8_t addr_try = (MPU6050_ADDR_A_7BIT << 1);

    // 1. Adres Denemesi (0x68)
    if (HAL_I2C_IsDeviceReady(hi2c, addr_try, 3, 100) == HAL_OK) {
        dynamic_mpu6050_addr = addr_try;
    }
    // 2. Adres Denemesi (0x69)
    else {
        addr_try = (MPU6050_ADDR_B_7BIT << 1);
        if (HAL_I2C_IsDeviceReady(hi2c, addr_try, 3, 100) == HAL_OK) {
            dynamic_mpu6050_addr = addr_try;
        } else {
            return 0; // İki adreste de cihaz cevap vermedi
        }
    }

    // WHO_AM_I Kontrolü
    if (HAL_I2C_Mem_Read(hi2c, dynamic_mpu6050_addr, REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &who, 1, 100) != HAL_OK)
        return 0;

    // MPU6050 için 0x68 doğrulaması (bazı klon çipler farklı dönebilir diye engellemiyoruz)
    if (who != 0x68 && who != 0x71 && who != 0x73) {
        // İhtiyaç duyarsan OLED debug basabilirsin:
        // char msg[20]; snprintf(msg, sizeof(msg), "W:%02X", who); displayScreen(msg);
    }

    // Uyku modunu kapat (Sleep = 0)
    uint8_t pwr_mgmt = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, dynamic_mpu6050_addr, REG_PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT, &pwr_mgmt, 1, 100) != HAL_OK)
        return 0;

    HAL_Delay(50);

    // Temel Örnekleme Hızı ve Filtre Ayarları
    uint8_t config_vals[4] = {0x07, 0x06, 0x00, 0x00};
    HAL_I2C_Mem_Write(hi2c, dynamic_mpu6050_addr, REG_SMPLRT_DIV,   I2C_MEMADD_SIZE_8BIT, &config_vals[0], 1, 100);
    HAL_I2C_Mem_Write(hi2c, dynamic_mpu6050_addr, REG_CONFIG,       I2C_MEMADD_SIZE_8BIT, &config_vals[1], 1, 100);
    HAL_I2C_Mem_Write(hi2c, dynamic_mpu6050_addr, REG_GYRO_CONFIG,  I2C_MEMADD_SIZE_8BIT, &config_vals[2], 1, 100);
    HAL_I2C_Mem_Write(hi2c, dynamic_mpu6050_addr, REG_ACCEL_CONFIG, I2C_MEMADD_SIZE_8BIT, &config_vals[3], 1, 100);

    return 1;
}


uint8_t MPU6050_Read6Axis(I2C_HandleTypeDef *hi2c, int16_t *accel, int16_t *gyro)
{
    uint8_t b[14];

    // İvmeölçer(6B) + Sıcaklık(2B) + Jiroskop(6B) = 14 Bayt dinamik adresten okunur
    if (HAL_I2C_Mem_Read(hi2c, dynamic_mpu6050_addr, REG_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, b, 14, 100) != HAL_OK)
    {
        return 0;
    }

    // İvme Eksensel Dağılım
    accel[0] = (int16_t)((b[0] << 8) | b[1]);
    accel[1] = (int16_t)((b[2] << 8) | b[3]);
    accel[2] = (int16_t)((b[4] << 8) | b[5]);

    // Jiroskop Eksensel Dağılım
    gyro[0] = (int16_t)((b[8]  << 8) | b[9]);
    gyro[1] = (int16_t)((b[10] << 8) | b[11]);
    gyro[2] = (int16_t)((b[12] << 8) | b[13]);

    return 1;
}

// ====================================================================
// MANYETOMETRE VE DÖNÜŞÜM FONKSİYONLARI (Bozulmadı)
// ====================================================================

uint8_t AK8963_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t who = 0;

    if (HAL_I2C_IsDeviceReady(hi2c, AK8963_ADDR, 3, 100) != HAL_OK)
        return 0;

    if (!ak_read_regs(hi2c, AK8963_WIA, &who, 1))
        return 0;

    if (!ak_write_reg(hi2c, AK8963_CNTL2, 0x01)) return 0;
    HAL_Delay(10);

    if (!ak_write_reg(hi2c, AK8963_CNTL1, 0x00)) return 0;
    HAL_Delay(10);

    if (!ak_write_reg(hi2c, AK8963_CNTL1, 0x16)) return 0;
    HAL_Delay(10);

    return 1;
}

uint8_t AK8963_ReadRaw(I2C_HandleTypeDef *hi2c, AK8963_Raw *out)
{
    uint8_t st1;
    uint8_t b[7];

    if (!ak_read_regs(hi2c, AK8963_ST1, &st1, 1))
        return 0;

    if ((st1 & 0x01) == 0)
        return 0;

    if (!ak_read_regs(hi2c, AK8963_XOUT_L, b, 7))
        return 0;

    out->mx = (int16_t)((b[1] << 8) | b[0]);
    out->my = (int16_t)((b[3] << 8) | b[2]);
    out->mz = (int16_t)((b[5] << 8) | b[4]);

    return 1;
}

void ATTITUDE_FromAccelMag(const MPU9250_Raw *imu, const AK8963_Raw *mag, Attitude_t *att)
{
    float ax = (float)imu->ax;
    float ay = (float)imu->ay;
    float az = (float)imu->az;

    float mx =  (float)mag->my;
    float my =  (float)mag->mx;
    float mz =  (float)mag->mz;

    float roll  = atan2f(ay, az);
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

    float cosr = cosf(roll), sinr = sinf(roll);
    float cosp = cosf(pitch), sinp = sinf(pitch);

    float Xh = mx * cosp + mz * sinp;
    float Yh = mx * sinr * sinp + my * cosr - mz * sinr * cosp;

    float yaw = atan2f(Yh, -Xh);
    att->yaw = yaw * 57.2958f;
    if (att->yaw < 0) att->yaw += 360.0f;
    att->yaw += 5.0f;
    if (att->yaw >= 360.0f) att->yaw -= 360.0f;

    att->roll  = roll  * 57.2958f;
    att->pitch = pitch * 57.2958f;
}

uint8_t MPU9250_GetAttitude(I2C_HandleTypeDef *hi2c, Attitude_t *att)
{
    MPU9250_Raw imu;
    AK8963_Raw mag;

    if (!MPU9250_ReadRaw(hi2c, &imu)) return 1;
    if (!AK8963_ReadRaw(hi2c, &mag)) return 2;

    float ax = (float)imu.ax;
    float ay = (float)imu.ay;
    float az = (float)imu.az;

    att->pitch = atan2f(ay, az) * 180.0f / M_PI;
    att->roll = atan2f(-ax, sqrtf(ay * ay + az * az)) * (-180.0f / M_PI);

    float mxOffset = -60.0f;
    float myOffset = 350.0f;

    float mx_cal = (float)mag.mx - mxOffset;
    float my_cal = (float)mag.my - myOffset;

    att->yaw = atan2f(mx_cal, my_cal) * 180.0f / M_PI;
    att->yaw -= 75.0f;

    if (att->yaw < 0) att->yaw += 360.0f;

    return 3;
}
