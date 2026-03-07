#ifndef INC_GPSSYSTEM_H_
#define INC_GPSSYSTEM_H_

#include "main.h"
#include "cmsis_os.h"   // osDelay
#include <stdint.h>

typedef struct {
  double lat;
  double lon;
  double alt;
  double spd_kmh;
  double cog_deg;
  int    fix;
  int    sats;
  char   rmc_status; // 'A' or 'V'
} GPS_Data;

void gpsSystem_Init(UART_HandleTypeDef *huart);
void gpsSystem_OnRxByte(uint8_t b);     // ISR/callback içinden çağrılacak
void gpsSystem_TaskStep(void);          // task içinde sık çağır
void gpsSystem_Get(GPS_Data *out);      // en güncel veriyi kopyalar
void gpsSystem_OnUartRxCplt(UART_HandleTypeDef *huart);

#endif
