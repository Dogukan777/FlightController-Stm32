#ifndef INC_FLIGHTCONTROLLER_H_
#define INC_FLIGHTCONTROLLER_H_
#include "SH1106.h"
#include "fonts.h"
#include "serialManager.h"
#include "wpManager.h"
#include <stdio.h>
#include <storage.h>
#include "gyroscope.h"
#include "cmsis_os.h"
#include "compass.h"
#include "controlPPM.h"
#include "gpsSystem.h"
#include "settings.h"
#include "servoMotor.h"
#include "stepMotor.h"
#include "raspiManager.h"

void I2C_Bus_Reset(I2C_HandleTypeDef *hi2c);
void I2C2_ScanDevices(void);
void StartSMTask(void *argument);
void StartUploadedWPTask(void *argument);
void StartPPMTask(void *argument);
void StartReadSensorTask(void *argument);
void StartSensorFailSafeTask(void *argument);
void StartRPITask(void *argument);
void flightController_Init(void);



extern osThreadId_t SMTaskHandle;
extern osThreadId_t UploadedWPTaskHandle;
extern osThreadId_t ppmTaskHandle;
extern osThreadId_t sensorTaskHandle;
extern osThreadId_t rpiTaskHandle;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
#endif /* INC_FLIGHTCONTROLLER_H_ */
