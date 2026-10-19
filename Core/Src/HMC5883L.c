/*
 * HMC5883L.c
 *
 *  Created on: Jan 18, 2022
 *      Author: hussam
 */


#include "HMC5883L.h"
#include "stm32f4xx.h"
#include "i2c.h"

uint8_t gaus_data[6];
#define mode_register (0x02)
#define config_b      (0x01)
#define address       (0x1E)
float m_Scale;

int8_t HMC5883L_Init(HMC5883LScale_t scale)
	{
	int8_t state;
	i2c_writeByte(address, mode_register, 0x00);

	if(state==-1){return -1;}

	i2c_writeByte(address, 0x00, 0xD0);

	if(state==-1){return -1;}

	switch(scale)
	    {
		case HMC5883L_RANGE_0_88GA:
			m_Scale = 0.073f;
		    break;

		case HMC5883L_RANGE_1_3GA:
			m_Scale = 0.92f;
		    break;

		case HMC5883L_RANGE_1_9GA:
			m_Scale = 1.2f;
		    break;

		case HMC5883L_RANGE_2_5GA:
			m_Scale = 1.52f;
		    break;

		case HMC5883L_RANGE_4GA:
			m_Scale = 2.27f;
		    break;

		case HMC5883L_RANGE_4_7GA:
			m_Scale = 2.56f;
		    break;

		case HMC5883L_RANGE_5_7GA:
			m_Scale = 3.03f;
		    break;

		case HMC5883L_RANGE_8_1GA:
			m_Scale = 4.35f;
		    break;

		default:
		    break;
	    }

	state = i2c_writeByte(address, config_b, scale);
	if(state==-1){return -1;}
	return 0;
	}

int8_t update_HMC5883L(void)
	{

	int8_t state;
	state=i2c_ReadMulti(address, 0x03, 6, (char *)gaus_data);
	if(state==-1){return -1;}
	return 0;
	}

float getGausX(void)
	{
	uint16_t data=gaus_data[0]<<8|gaus_data[1];
	float g=data*m_Scale;
	return g;
	}

float getGausY(void)
	{
	uint16_t data=gaus_data[4]<<8|gaus_data[5];
	float g=data*m_Scale;
	return g;
	}

float getGausZ(void)
	{
	uint16_t data=gaus_data[2]<<8|gaus_data[3];
	float g=data*m_Scale;
	return g;
	}
