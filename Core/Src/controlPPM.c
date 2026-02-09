#include "main.h"
#include "controlPPM.h"
#include "cmsis_os.h"
#include "flightController.h"
#include <stdio.h>

volatile uint16_t ppm_ch[PPM_CH_MAX] = {1500};
volatile uint8_t  ppm_ch_count = 0;

volatile uint32_t last_cap = 0;
volatile uint8_t  ch_idx = 0;
volatile uint32_t ppm_last_ms = 0;
static uint16_t thr_f = 1000;
static uint16_t last_out = 1000;
static uint32_t t_esc = 0;
extern volatile uint32_t tim2_ic_cnt;


void controlPPM_Init(){
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	ESC_WriteUs(1000);
	HAL_Delay(3000);
	ppm_last_ms = HAL_GetTick();
	last_cap = 0;
	ch_idx = 0;
	ppm_ch_count = 0;
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

void controlPPM_Start(){

	 uint32_t now = HAL_GetTick();
		    if (now - t_esc >= 5) {
		      t_esc = now;
		      uint16_t thr = ppm_ch[THROTTLE_CH_INDEX];
		      if (thr < 1000) thr = 1000;
		      if (thr > 2000) thr = 2000;
		      thr_f = (uint16_t)((thr_f * 19 + thr) / 20);   // daha yumuşak (0.95)
		      uint16_t out = thr_f;
		      if ((out > last_out && out - last_out <= 2) || (last_out > out && last_out - out <= 2)) {
		        out = last_out; // deadband 2us
		      }
		      last_out = out;

		      ESC_WriteUs(last_out);     // map etme gerek yok, direkt 1000..2000 yolla
		    }
}

void ESC_WriteUs(uint16_t us)
{
  if (us < 1000) us = 1000;
  if (us > 2000) us = 2000;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, us);
}



/* 1000..2000us -> -100..+100 (%), 1500us -> 0% */
int16_t ppm_to_percent(uint16_t us)
{
  if (us < 1000) us = 1000;
  if (us > 2000) us = 2000;
  return (int16_t)(((int32_t)us - 1500) * 100 / 500);
}
void displayScreen(const char *str)
{
  SH1106_Clear();
  SH1106_GotoXY(5, 5);
  SH1106_Puts((char*)str, &Font_7x10, 1);
  SH1106_UpdateScreen();

  if (osKernelGetState() == osKernelRunning) osDelay(5);
  else HAL_Delay(5);
}
