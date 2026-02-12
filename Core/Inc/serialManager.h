#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static const char *CONNECT_WORD = "CONNECT";
static const char *DISCONNECT_WORD = "DISCONNECT";
static uint8_t disconnectMatchIndex = 0;
static uint8_t connectMatchIndex = 0;
static volatile uint8_t gotConnectFlag = 0;
static volatile uint8_t gotDisconnectFlag = 0;
static volatile uint8_t sm_connected = 0;
extern volatile uint8_t data_stream_enabled;
// Başlatma: huart2 adresini ver
void SM_Init(UART_HandleTypeDef *huart);
void SM_Start();
// Gönderme (string)
HAL_StatusTypeDef SM_SendString(const char *s);

// Gönderme (buffer)
HAL_StatusTypeDef SM_SendBytes(const uint8_t *data, uint16_t len);

// --- RX (Interrupt + buffer) ---
// Buffer'da kaç byte var?
uint16_t SM_Available(void);

// 1 byte oku (varsa 1 döner)
uint8_t SM_ReadByte(uint8_t *out);

// Satır oku: '\n' gelene kadar (LF). Çıkış null-terminated.
// return: okunan karakter sayısı (0 = henüz satır yok)
uint16_t SM_ReadLine(char *out, uint16_t maxLen);
uint8_t SM_GotDisconnect(void);
uint8_t SM_IsConnected(void);
// "Connect" geldi mi? (case-sensitive)
uint8_t SM_GotConnect(void);

// UART RX interrupt callback içinde çağır
void SM_RxCpltCallback(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif // SERIALMANAGER_H
