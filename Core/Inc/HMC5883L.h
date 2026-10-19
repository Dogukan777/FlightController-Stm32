/*
 * HMC5883L.h
 *
 *  Created on: Jan 18, 2022
 *      Author: hussam
 */

#ifndef HMC5883L_H_
#define HMC5883L_H_
#include "stdint.h"


typedef enum
	{
	HMC5883L_RANGE_0_88GA	=0x00,
	HMC5883L_RANGE_1_3GA	=0x20,
	HMC5883L_RANGE_1_9GA	=0x40,
	HMC5883L_RANGE_2_5GA    =0x60,
	HMC5883L_RANGE_4GA      =0x80,
	HMC5883L_RANGE_4_7GA    =0xA0,
	HMC5883L_RANGE_5_7GA    =0xC0,
	HMC5883L_RANGE_8_1GA    =0xE0

	}HMC5883LScale_t;

int8_t update_HMC5883L(void);
int8_t HMC5883L_Init(HMC5883LScale_t scale);
float getGausX(void);
float getGausY(void);
float getGausZ(void);



#endif /* HMC5883L_H_ */
