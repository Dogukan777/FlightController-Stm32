#include "raspiManager.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *g_rpi_huart = NULL;

// Ring Buffer Değişkenleri
static volatile uint8_t  rpi_rxBuf[RPI_RX_BUF_SIZE];
static volatile uint16_t rpi_rxHead = 0;
static volatile uint16_t rpi_rxTail = 0;

static uint8_t rpi_rxByte = 0;

/* --- Ring Buffer Yardımcı Fonksiyonları --- */
static void rpi_rb_push(uint8_t b)
{
    uint16_t next = (uint16_t)((rpi_rxHead + 1) % RPI_RX_BUF_SIZE);
    if (next == rpi_rxTail) {
        // Tampon dolduğunda en eskiyi düşür
        rpi_rxTail = (uint16_t)((rpi_rxTail + 1) % RPI_RX_BUF_SIZE);
    }
    rpi_rxBuf[rpi_rxHead] = b;
    rpi_rxHead = next;
}

static uint8_t rpi_rb_pop(uint8_t *out)
{
    if (rpi_rxHead == rpi_rxTail) return 0; // Tampon boş
    *out = rpi_rxBuf[rpi_rxTail];
    rpi_rxTail = (uint16_t)((rpi_rxTail + 1) % RPI_RX_BUF_SIZE);
    return 1;
}

/* Tam Satır Okuma (\n görene kadar bekler, \r karakterini eler) */
static uint16_t RPI_ReadLine(char *out, uint16_t maxLen)
{
    if (!out || maxLen < 2) return 0;

    uint16_t t = rpi_rxTail;
    uint16_t count = 0;
    uint8_t foundNL = 0;

    // Ring buffer'ı bozmadan satır sonu (\n) var mı kontrol et
    while (t != rpi_rxHead && count < (maxLen - 1)) {
        uint8_t c = rpi_rxBuf[t];
        if (c == '\n') { foundNL = 1; break; }
        t = (uint16_t)((t + 1) % RPI_RX_BUF_SIZE);
        count++;
    }

    if (!foundNL) return 0; // Satır henüz tamamlanmadı

    uint16_t written = 0;
    uint8_t c;

    while (written < (maxLen - 1)) {
        if (!rpi_rb_pop(&c)) break;
        if (c == '\r') continue;
        if (c == '\n') break;

        out[written++] = (char)c;
    }

    out[written] = '\0';
    return written;
}

/* --- Dışa Açık Fonksiyonlar --- */

void RPI_Init(UART_HandleTypeDef *huart)
{
    g_rpi_huart = huart;
    rpi_rxHead = 0;
    rpi_rxTail = 0;

    // Kesme (IT) ile tek byte okumayı başlat (UART2)
    HAL_UART_Receive_IT(g_rpi_huart, &rpi_rxByte, 1);
}

HAL_StatusTypeDef RPI_SendBytes(const uint8_t *data, uint16_t len)
{
    if (!g_rpi_huart || !data || len == 0) return HAL_ERROR;
    return HAL_UART_Transmit(g_rpi_huart, (uint8_t*)data, len, 1000);
}

HAL_StatusTypeDef RPI_SendString(const char *s)
{
    if (!s) return HAL_ERROR;
    return RPI_SendBytes((const uint8_t*)s, (uint16_t)strlen(s));
}

// UART RX Interrupt Callback
void RPI_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (!g_rpi_huart || huart != g_rpi_huart) return;

    // Gelen byte'ı tampona yaz
    rpi_rb_push(rpi_rxByte);

    // Sonraki byte için kesmeyi tekrar kur
    HAL_UART_Receive_IT(g_rpi_huart, &rpi_rxByte, 1);
}

/* Raspberry Pi Komut İşleme Mantığı */
void RPI_ProcessCommands(void)
{
    char line[128];

    while (RPI_ReadLine(line, sizeof(line)) > 0)
    {
    	SM_SendString(line);
        // 1. LED_ON Komutu (Örn: LED_ON,2000)
        if (strncmp(line, "LED_ON,", 7) == 0)
        {
            int sure_ms = 0;
            if (sscanf(line, "LED_ON,%d", &sure_ms) == 1)
            {
                // LED süresine göre işlemlerini buraya yazabilirsin
                // Örn: LED_Atsin(sure_ms);
            	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
            	//osDelay(sure_ms); // 1 saniye HIGH/3.3V tut
            	 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
            	 //osDelay(20);

            }
        }
        // 2. SERVO Komutu (Örn: SERVO,1,20 -> 1 nolu servo, 20 derece aci)
        else if (strncmp(line, "SERVO,", 6) == 0)
        {
            int servo_aci = 0;
            int servo_sure = 0;
            if (sscanf(line, "SERVO,%d,%d", &servo_aci, &servo_sure) == 2)
            {
                // Servo açı sürme fonksiyonunu buraya ekleyebilirsin
                // Örn: Servo_SetAngle(servo_id, aci);
            	 Servo_WriteAngle(servo_aci);
            	 //osDelay(servo_sure);
            }
        }
        // 3. STEP Komutu (Örn: STEP,1,20 -> 1 nolu step motor, 20 ms donme suresi)
        else if (strncmp(line, "STEP,", 5) == 0)
        {
            int step_aci = 0;
            int step_sure = 0;
            if (sscanf(line, "STEP,%d,%d", &step_aci, &step_sure) == 2)
            {
            	  Servo_WriteAngle(step_aci);
            	  //osDelay(step_sure);
            }
        }
        else
        {
            RPI_SendString("ERR:UNKNOWN_CMD\n");
            SM_SendString("ERR:UNKNOWN_CMD\n");
        }
        SM_SendString("ERR:UNKNOWN_CMD22\n");
    }

}
