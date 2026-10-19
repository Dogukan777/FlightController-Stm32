#ifndef STEPMOTOR_H
#define STEPMOTOR_H

#include "stm32f4xx_hal.h"
#include "main.h"

// STEP Pini: PB0 (GPIOB)
#define STEP_PORT GPIOB
#define STEP_PIN  GPIO_PIN_0

// DIR Pini: PC1 (GPIOC)
#define DIR_PORT  GPIOC
#define DIR_PIN   GPIO_PIN_1

// Motor Parametreleri (Varsayılan: Full step 1.8 derece = 200 adım/tur)
#define STEPS_PER_REV 200

// Fonksiyon Bildirimleri
void StepMotor_Init(void);
void StepMotor_Step(uint16_t steps, uint8_t direction, uint16_t delay_us);
void StepMotor_RotateAngle(float angle, uint16_t delay_us);

#endif /* STEPMOTOR_H */
