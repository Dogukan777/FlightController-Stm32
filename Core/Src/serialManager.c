#include "serialManager.h"
#include <string.h>

// ---- Ayarlar ----
#define SM_RX_BUF_SIZE 256

static UART_HandleTypeDef *g_huart = NULL;

// Ring buffer
static volatile uint8_t  rxBuf[SM_RX_BUF_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

// Interrupt ile tek byte alma değişkeni
static uint8_t rxByte = 0;

// "Connect" tespit için küçük state



static void rb_push(uint8_t b)
{
    uint16_t next = (uint16_t)((rxHead + 1) % SM_RX_BUF_SIZE);
    if (next == rxTail) {
        // overflow: en eskiyi düşür
        rxTail = (uint16_t)((rxTail + 1) % SM_RX_BUF_SIZE);
    }
    rxBuf[rxHead] = b;
    rxHead = next;
}

static uint8_t rb_pop(uint8_t *out)
{
    if (rxHead == rxTail) return 0; // empty
    *out = rxBuf[rxTail];
    rxTail = (uint16_t)((rxTail + 1) % SM_RX_BUF_SIZE);
    return 1;
}

static uint16_t rb_count(void)
{
    if (rxHead >= rxTail) return (uint16_t)(rxHead - rxTail);
    return (uint16_t)(SM_RX_BUF_SIZE - rxTail + rxHead);
}

void SM_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    rxHead = rxTail = 0;
    connectMatchIndex = 0;
    gotConnectFlag = 0;
    disconnectMatchIndex = 0;
    gotDisconnectFlag = 0;
    sm_connected = 0;

    // RX interrupt ile 1 byte almaya başla
    HAL_UART_Receive_IT(g_huart, &rxByte, 1);
}

uint8_t SM_GotDisconnect(void)
{
    if (gotDisconnectFlag) {
        gotDisconnectFlag = 0;
        return 1;
    }
    return 0;
}

uint8_t SM_IsConnected(void)
{
    return sm_connected;
}


HAL_StatusTypeDef SM_SendBytes(const uint8_t *data, uint16_t len)
{
    if (!g_huart || !data || len == 0) return HAL_ERROR;
    // Timeout 100ms * len gibi düşün; basitçe 1000ms verelim
    return HAL_UART_Transmit(g_huart, (uint8_t*)data, len, 1000);
}

HAL_StatusTypeDef SM_SendString(const char *s)
{
    if (!s) return HAL_ERROR;
    return SM_SendBytes((const uint8_t*)s, (uint16_t)strlen(s));
}

uint16_t SM_Available(void)
{
    return rb_count();
}

uint8_t SM_ReadByte(uint8_t *out)
{
    if (!out) return 0;
    return rb_pop(out);
}

// '\n' gelene kadar okur (LF). '\r' (CR) gelirse yok sayar.
uint16_t SM_ReadLine(char *out, uint16_t maxLen)
{
    if (!out || maxLen < 2) return 0;

    // Tail'i bozmadan satır var mı kontrol etmek için geçici indeks
    uint16_t t = rxTail;
    uint16_t count = 0;
    uint8_t foundNL = 0;

    while (t != rxHead && count < (maxLen - 1)) {
        uint8_t c = rxBuf[t];
        if (c == '\n') { foundNL = 1; break; }
        t = (uint16_t)((t + 1) % SM_RX_BUF_SIZE);
        count++;
    }

    if (!foundNL) return 0; // satır tamam değil

    // Artık gerçekten okuyabiliriz
    uint16_t written = 0;
    uint8_t c;

    while (written < (maxLen - 1)) {
        if (!rb_pop(&c)) break;

        if (c == '\r') continue;
        if (c == '\n') break;

        out[written++] = (char)c;
    }

    out[written] = '\0';
    return written;
}

uint8_t SM_GotConnect(void)
{
    if (gotConnectFlag) {
        gotConnectFlag = 0;
        return 1;
    }
    return 0;
}

// Bu fonksiyonu stm32f4xx_it.c içindeki HAL_UART_RxCpltCallback'ten çağıracağız
void SM_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (!g_huart || huart != g_huart) return;

    // Gelen byte:
    uint8_t b = rxByte;

    // Ring buffer'a koy
    rb_push(b);

    // 1) DISCONNECT matcher önce
    if (b == (uint8_t)DISCONNECT_WORD[disconnectMatchIndex]) {
        disconnectMatchIndex++;
        if (DISCONNECT_WORD[disconnectMatchIndex] == '\0') {
            gotDisconnectFlag = 1;
            disconnectMatchIndex = 0;

            // 🔥 kritik: connect eşleşmesini sıfırla
            connectMatchIndex = 0;

            HAL_UART_Receive_IT(g_huart, &rxByte, 1);
            return; // 🔥 bu byte için CONNECT kontrol etme
        }
    } else {
        disconnectMatchIndex = 0;
        if (b == (uint8_t)DISCONNECT_WORD[0]) {
            disconnectMatchIndex = 1;
        }
    }

    // 2) CONNECT matcher sonra
    if (b == (uint8_t)CONNECT_WORD[connectMatchIndex]) {
        connectMatchIndex++;
        if (CONNECT_WORD[connectMatchIndex] == '\0') {
            gotConnectFlag = 1;
            connectMatchIndex = 0;
        }
    } else {
        connectMatchIndex = 0;
        if (b == (uint8_t)CONNECT_WORD[0]) {
            connectMatchIndex = 1;
        }
    }



    // Bir sonraki byte için tekrar başlat
    HAL_UART_Receive_IT(g_huart, &rxByte, 1);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    SM_RxCpltCallback(huart);
}
