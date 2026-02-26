#ifndef INC_FLIGHTCONTROLLER_H_
#define INC_FLIGHTCONTROLLER_H_
#include "SH1106.h"
#include "fonts.h"
#include "serialManager.h"
#include "wpManager.h"
#include <stdio.h>
#include "wpStorage.h"
#include "gyroscope.h"
#include "cmsis_os.h"


void StartSMTask(void *argument);
void StartUploadedWPTask(void *argument);
void StartGyroTask(void *argument);
void StartPPMTask(void *argument);
void StartGPSTask(void *argument);

void flightController_Init(void);

void displayScreen(const char *str);
void displayTwoLines(const char *top, const char *bottom);



extern osThreadId_t SMTaskHandle;
extern osThreadId_t UploadedWPTaskHandle;
extern osThreadId_t gyroTaskHandle;
extern osThreadId_t ppmTaskHandle;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
#endif /* INC_FLIGHTCONTROLLER_H_ */
