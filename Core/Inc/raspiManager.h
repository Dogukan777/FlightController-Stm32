#ifndef INC_RASPIMANAGER_H_
#define INC_RASPIMANAGER_H_

#include "main.h"
#include <stdint.h>
#include "serialManager.h"

#define RPI_RX_BUF_SIZE 256

// Fonksiyon Prototipleri
void RPI_Init(UART_HandleTypeDef *huart);
void RPI_ProcessCommands(void);
HAL_StatusTypeDef RPI_SendString(const char *s);
HAL_StatusTypeDef RPI_SendBytes(const uint8_t *data, uint16_t len);

// UART Interrupt Handler içerisinden çağrılacak callback
void RPI_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* INC_RASPIMANAGER_H_ */
