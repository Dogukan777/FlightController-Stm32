#include "QMC5883.h"
#include <math.h>

uint8_t QMC5883_Init(QMC_HandleTypedef *qmc, I2C_HandleTypeDef *i2c, QMC_DataRate_t data_rate)
{
    uint8_t reg_rst = 0x01;   // set/reset period register
    uint8_t ctrl    = 0x01;   // continuous mode

    qmc->i2c = i2c;
    qmc->ctl_reg = 0x11;

    if (data_rate == QMC_Rate_200)      ctrl |= 0b00001100;
    else if (data_rate == QMC_Rate_100) ctrl |= 0b00001000;
    else if (data_rate == QMC_Rate_50)  ctrl |= 0b00000100;
    else if (data_rate == QMC_Rate_10)  ctrl |= 0b00000000;
    else                                ctrl |= 0b00000100;

    qmc->ctl_reg = ctrl;

    if (HAL_I2C_Mem_Write(qmc->i2c, QMC_I2C_Address, 0x0B, I2C_MEMADD_SIZE_8BIT, &reg_rst, 1, 100) != HAL_OK)
        return QMC_ERR;

    if (HAL_I2C_Mem_Write(qmc->i2c, QMC_I2C_Address, 0x09, I2C_MEMADD_SIZE_8BIT, &ctrl, 1, 100) != HAL_OK)
        return QMC_ERR;

    return QMC_OK;
}

uint8_t QMC5883_Read(QMC_HandleTypedef *qmc)
{
    uint8_t status = 0;

    if (HAL_I2C_Mem_Read(qmc->i2c, QMC_I2C_Address, 0x06, I2C_MEMADD_SIZE_8BIT, &status, 1, 100) != HAL_OK)
        return QMC_ERR;

    // data ready bit
    if ((status & 0x01) == 0)
        return QMC_ERR;

    if (HAL_I2C_Mem_Read(qmc->i2c, QMC_I2C_Address, 0x00, I2C_MEMADD_SIZE_8BIT, qmc->data, 6, 100) != HAL_OK)
        return QMC_ERR;

    qmc->xaxis = (int16_t)((qmc->data[1] << 8) | qmc->data[0]);
    qmc->yaxis = (int16_t)((qmc->data[3] << 8) | qmc->data[2]);
    qmc->zaxis = (int16_t)((qmc->data[5] << 8) | qmc->data[4]);

    qmc->compass = atan2f((float)qmc->yaxis, (float)qmc->xaxis) * 180.0f / (float)M_PI;

    if (qmc->compass >= 0.0f)
        qmc->heading = qmc->compass;
    else
        qmc->heading = 360.0f + qmc->compass;

    qmc->avg_compass = qmc->compass;

    return QMC_OK;
}

float QMC5883_ReadHeading(QMC_HandleTypedef *qmc)
{
    if (QMC5883_Read(qmc) != QMC_OK)
        return -1.0f;

    return qmc->heading;
}

uint8_t QMC5883_ReadAverage(QMC_HandleTypedef *qmc, uint32_t maxAverage, uint32_t timePerAvg)
{
    float avgH = 0.0f;
    float avgC = 0.0f;
    uint32_t okCount = 0;

    if (maxAverage == 0)
        return QMC_ERR;

    for (uint32_t i = 0; i < maxAverage; i++)
    {
        if (QMC5883_Read(qmc) == QMC_OK)
        {
            avgC += qmc->compass;
            avgH += qmc->heading;
            okCount++;
        }

        HAL_Delay(timePerAvg);
    }

    if (okCount == 0)
        return QMC_ERR;

    qmc->avg_heading = avgH / (float)okCount;
    qmc->avg_compass = avgC / (float)okCount;

    return QMC_OK;
}

uint8_t QMC5883_Standby(QMC_HandleTypedef *qmc)
{
    uint8_t reg = 0x00; // standby mode
    if (HAL_I2C_Mem_Write(qmc->i2c, QMC_I2C_Address, 0x09, I2C_MEMADD_SIZE_8BIT, &reg, 1, 100) != HAL_OK)
        return QMC_ERR;
    return QMC_OK;
}

uint8_t QMC5883_Reset(QMC_HandleTypedef *qmc)
{
    uint8_t reg = 0x80; // soft reset
    if (HAL_I2C_Mem_Write(qmc->i2c, QMC_I2C_Address, 0x0A, I2C_MEMADD_SIZE_8BIT, &reg, 1, 100) != HAL_OK)
        return QMC_ERR;
    return QMC_OK;
}
