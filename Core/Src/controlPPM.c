#include "main.h"
#include "controlPPM.h"
#include "cmsis_os.h"
#include "flightController.h"
#include <stdio.h>
#include "settings.h"

volatile uint16_t ppm_ch[PPM_CH_MAX] = {1500};
volatile uint8_t  ppm_ch_count = 0;

volatile uint32_t last_cap = 0;
volatile uint8_t  ch_idx = 0;
volatile uint32_t ppm_last_ms = 0;
static uint16_t thr_f = 1000;
static uint16_t last_out = 1000;
static uint32_t t_esc = 0;
extern volatile uint32_t tim2_ic_cnt;

void controlPPM_Init()
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    ESC_WriteUs(900);

    ppm_last_ms = HAL_GetTick();
    last_cap = 0;
    ch_idx = 0;
    ppm_ch_count = 0;

    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

void controlPPM_Start()
{
    uint32_t now = HAL_GetTick();
    Servo_UpdateFromYaw();
    Servo_UpdateFromPitch();
    Servo_UpdateFromRoll();
    if (now - t_esc >= 5) {
        t_esc = now;

        uint16_t thr = ppm_ch[THROTTLE_CH_INDEX];
        if (thr < 1000) thr = 1000;
        if (thr > 2000) thr = 2000;

        thr_f = (uint16_t)((thr_f * 19 + thr) / 20);
        uint16_t out = thr_f;

        if ((out > last_out && out - last_out <= 2) ||
            (last_out > out && last_out - out <= 2)) {
            out = last_out;
        }

        last_out = out;
        ESC_WriteUs(last_out);
    }
}

void ESC_WriteUs(uint16_t us)
{
    if (us < 1000) us = 1000;
    if (us > 2000) us = 2000;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, us);
}

/* 1000..2000us -> -100..+100 */
int16_t ppm_to_percent(uint16_t us)
{
    if (us < 1000) us = 1000;
    if (us > 2000) us = 2000;

    return (int16_t)(((int32_t)us - 1500) * 100 / 500);
}

/* -100..+100 -> 1000..2000us */
uint16_t percent_to_pwm(int16_t percent)
{
    if (percent < -100) percent = -100;
    if (percent > 100)  percent = 100;

    return (uint16_t)(1500 + (percent * 5));
}

void Servo_UpdateFromPitch(void)
{
    int16_t pitch_percent;
    uint16_t servo_pwm;

    pitch_percent = ppm_to_percent(ppm_ch[PITCH_CH_INDEX]);
    servo_pwm = percent_to_pwm(pitch_percent);

    g_settings.servo[1].inst = servo_pwm;   // Servo2 = TIM3_CH2
    Servo_ApplyByIndex(1);
}

void Servo_UpdateFromRoll(void)
{
    int16_t roll_percent;
    uint16_t servo_pwm;

    roll_percent = ppm_to_percent(ppm_ch[ROLL_CH_INDEX]);
    servo_pwm = percent_to_pwm(roll_percent);

    g_settings.servo[3].inst = servo_pwm;
    Servo_ApplyByIndex(3);
    g_settings.servo[2].inst = servo_pwm;
    Servo_ApplyByIndex(2);
}

void Servo_UpdateFromYaw(void)
{
    int16_t yaw_percent;
    uint16_t servo_pwm;

    yaw_percent = ppm_to_percent(ppm_ch[YAW_CH_INDEX]);
    servo_pwm = percent_to_pwm(yaw_percent);

    g_settings.servo[0].inst = servo_pwm;
    Servo_ApplyByIndex(0);
}


