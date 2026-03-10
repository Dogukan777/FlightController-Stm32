#ifndef SETTINGS_H
#define SETTINGS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t id;
    uint16_t max;
    uint16_t min;
    uint16_t inst;
} ServoSetting_t;

typedef struct {
    ServoSetting_t servo[4];
} SettingsPacket_t;

extern SettingsPacket_t g_settings;

void Settings_Init(void);
uint8_t Settings_ParseLine(const char *line);
ServoSetting_t Settings_GetServo(uint8_t index);
uint8_t Settings_HandleCommand(const char *line);
void Settings_PrintAllOLED(void);
void Settings_SendToQt(void);

#ifdef __cplusplus
}
#endif

#endif
