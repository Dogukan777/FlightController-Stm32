#ifndef COMPASS_H
#define COMPASS_H

#include "main.h"

#define HMC_ADDR (0x1E << 1)

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    float heading;
} HMC_Data;

uint8_t HMC_Init(I2C_HandleTypeDef *hi2c);
uint8_t HMC_Read(I2C_HandleTypeDef *hi2c, HMC_Data *data);

#endif
