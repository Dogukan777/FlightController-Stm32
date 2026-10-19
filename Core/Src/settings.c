#include "serialManager.h"
#include <string.h>
#include "gpsSystem.h"
#include <stdio.h>
#include "cmsis_os2.h"
#include "wpManager.h"
#include "storage.h"
#include "settings.h"
#include "flightController.h"

SettingsPacket_t g_settings;


void Settings_Init(void)
{
	g_settings.servo[0].id   = 1;
	g_settings.servo[0].max  = 2000;
	g_settings.servo[0].min  = 1000;
	g_settings.servo[0].inst = 1500;

	g_settings.servo[1].id   = 2;
	g_settings.servo[1].max  = 2000;
	g_settings.servo[1].min  = 1000;
	g_settings.servo[1].inst = 1500;

	g_settings.servo[2].id   = 3;
	g_settings.servo[2].max  = 2000;
	g_settings.servo[2].min  = 1000;
	g_settings.servo[2].inst = 1500;

	g_settings.servo[3].id   = 4;
	g_settings.servo[3].max  = 2000;
	g_settings.servo[3].min  = 1000;
	g_settings.servo[3].inst = 1500;

	if (Settings_LoadFromFlash()) {
		//displayTwoLines("SETTINGS", "LOADED");
	    //Settings_PrintAllOLED();
	} else {
	    //displayTwoLines("SETTINGS", "DEFAULT");
	    //Settings_PrintAllOLED();
	}
	Servo_StartAllAndApply();
}
void Settings_SendToQt(void)
{
    char msg[160];

    snprintf(msg, sizeof(msg),
             "SETTINGS_DATA,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
             g_settings.servo[0].id, g_settings.servo[0].max, g_settings.servo[0].min, g_settings.servo[0].inst,
             g_settings.servo[1].id, g_settings.servo[1].max, g_settings.servo[1].min, g_settings.servo[1].inst,
             g_settings.servo[2].id, g_settings.servo[2].max, g_settings.servo[2].min, g_settings.servo[2].inst,
             g_settings.servo[3].id, g_settings.servo[3].max, g_settings.servo[3].min, g_settings.servo[3].inst);

    SM_SendString(msg);
}


uint8_t Settings_ParseLine(const char *line)
{
    if (line == NULL) return 0;

    uint16_t v[16];

    int n = sscanf(line,
                   "SETTINGS,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu",
                   &v[0],  &v[1],  &v[2],  &v[3],
                   &v[4],  &v[5],  &v[6],  &v[7],
                   &v[8],  &v[9],  &v[10], &v[11],
                   &v[12], &v[13], &v[14], &v[15]);

    if (n != 16) {
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        g_settings.servo[i].id   = v[i * 4 + 0];
        g_settings.servo[i].max  = v[i * 4 + 1];
        g_settings.servo[i].min  = v[i * 4 + 2];
        g_settings.servo[i].inst = v[i * 4 + 3];
    }

    return 1;
}



ServoSetting_t Settings_GetServo(uint8_t index)
{
    ServoSetting_t empty = {0, 0, 0, 0};

    if (index >= 4) return empty;
    return g_settings.servo[index];
}

uint8_t Settings_HandleCommand(const char *line)
{
    if (line == NULL) return 0;

    if (strncmp(line, "SETTINGS,", 9) != 0) {
        return 0;
    }

    if (!Settings_ParseLine(line)) {
        return 0;
    }

    Servo_ApplyByIndex(0);
    Servo_ApplyByIndex(1);
    Servo_ApplyByIndex(2);
    Servo_ApplyByIndex(3);


    if (Settings_SaveToFlash()) {

    } else {

        return 0;
    }

    return 1;
}


ServoTimMap_t Settings_GetTim(uint8_t index)
{
    ServoTimMap_t t;

    /*switch (index) {
        case 0: t.htim = &htim3; t.channel = TIM_CHANNEL_1; break;
        case 1: t.htim = &htim3; t.channel = TIM_CHANNEL_2; break;
        case 2: t.htim = &htim4; t.channel = TIM_CHANNEL_3; break;
        case 3: t.htim = &htim4; t.channel = TIM_CHANNEL_4; break;
        default: t.htim = &htim3; t.channel = TIM_CHANNEL_1; break;
    }*/

    return t;
}

/*uint32_t Settings_GetTimChannel(uint8_t index)
{
    switch (index) {
        case 0: return TIM_CHANNEL_1; // servo1
        case 1: return TIM_CHANNEL_2; // servo2
        case 2: return TIM_CHANNEL_3; // servo3
        case 3: return TIM_CHANNEL_4; // servo4
        default: return TIM_CHANNEL_1;
    }
}*/
void Servo_ApplyByIndex(uint8_t index)
{
    if (index >= 4) return;

    ServoSetting_t s = g_settings.servo[index];
    uint16_t pulse = Settings_Clamp(s.inst, s.min, s.max);

    ServoTimMap_t t = Settings_GetTim(index);

    //__HAL_TIM_SET_COMPARE(t.htim, t.channel, pulse);
}
void Servo_StartAllAndApply(void)
{
	// TIM3
	//HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	//HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

	// TIM4
	//HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
	//HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

    Servo_ApplyByIndex(0);
    Servo_ApplyByIndex(1);
    Servo_ApplyByIndex(2);
    Servo_ApplyByIndex(3);
}
uint16_t Settings_Clamp(uint16_t value, uint16_t min, uint16_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void Servo_WriteByIndex(uint8_t index, uint16_t pwm)
{
    if (index >= 4) return;

    ServoSetting_t s = g_settings.servo[index];
    pwm = Settings_Clamp(pwm, s.min, s.max);

    ServoTimMap_t t = Settings_GetTim(index);

   // __HAL_TIM_SET_COMPARE(t.htim, t.channel, pwm);
}

uint16_t Settings_MapPercentToServo(uint8_t index, int16_t percent)
{
    if (index >= 4) return 1500;

    ServoSetting_t s = g_settings.servo[index];

    if (percent < -100) percent = -100;
    if (percent > 100)  percent = 100;

    if (percent == 0) {
        return s.inst;
    }

    if (percent > 0) {
        // 0..100  => inst..max
        uint32_t pwm = s.inst + ((int32_t)(s.max - s.inst) * percent) / 100;
        return Settings_Clamp((uint16_t)pwm, s.min, s.max);
    } else {
        // -100..0 => min..inst
        // percent negatif olduğu için toplama yapıyoruz
        uint32_t pwm = s.inst + ((int32_t)(s.inst - s.min) * percent) / 100;
        return Settings_Clamp((uint16_t)pwm, s.min, s.max);
    }
}


void Settings_PrintAllOLED(void)
{
    char top[32];
    char bottom[32];

    for (int i = 0; i < 4; i++) {
        snprintf(top, sizeof(top), "Servo ID:%u", g_settings.servo[i].id);
        snprintf(bottom, sizeof(bottom), "Mx:%u Mn:%u I:%u",
                 g_settings.servo[i].max,
                 g_settings.servo[i].min,
                 g_settings.servo[i].inst);

        displayTwoLines(top, bottom);
    }
}



