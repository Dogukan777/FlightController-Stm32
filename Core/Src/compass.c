#include "compass.h"
#include <math.h>

uint8_t HMC_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t data;

    // Config A (8 samples, 15Hz)
    data = 0x70;
    if (HAL_I2C_Mem_Write(hi2c, HMC_ADDR, 0x00, 1, &data, 1, 100) != HAL_OK)
        return 0;

    // Config B (gain)
    data = 0x20;
    if (HAL_I2C_Mem_Write(hi2c, HMC_ADDR, 0x01, 1, &data, 1, 100) != HAL_OK)
        return 0;

    // Mode (continuous)
    data = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, HMC_ADDR, 0x02, 1, &data, 1, 100) != HAL_OK)
        return 0;

    return 1;
}

uint8_t HMC_Read(I2C_HandleTypeDef *hi2c, HMC_Data *d)
{
    uint8_t buf[6];

    if (HAL_I2C_Mem_Read(hi2c, HMC_ADDR, 0x03, 1, buf, 6, 100) != HAL_OK)
        return 0;

    d->x = (int16_t)(buf[0] << 8 | buf[1]);
    d->z = (int16_t)(buf[2] << 8 | buf[3]);
    d->y = (int16_t)(buf[4] << 8 | buf[5]);

    float heading = atan2f((float)d->y, (float)d->x) * 180.0f / M_PI;

    if (heading < 0)
        heading += 360;

    d->heading = heading;

    return 1;
}
