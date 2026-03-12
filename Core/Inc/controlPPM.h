#ifndef CONTROL_PPM_H
#define CONTROL_PPM_H
#include <stdint.h>

#define PPM_CH_MAX          10
#define PPM_SYNC_US         3000
#define PPM_PULSE_MIN_US    800
#define PPM_PULSE_MAX_US    2200

#define ROLL_CH_INDEX       0
#define PITCH_CH_INDEX      1
#define THROTTLE_CH_INDEX   2
#define YAW_CH_INDEX        3



extern volatile uint16_t ppm_ch[PPM_CH_MAX];
extern volatile uint8_t  ppm_ch_count;
extern volatile uint32_t ppm_last_ms;

extern volatile uint32_t last_cap;
extern volatile uint8_t  ch_idx;

void controlPPM_Init(void);
void controlPPM_Start(void);
void ESC_WriteUs(uint16_t us);
int16_t ppm_to_percent(uint16_t us);
uint16_t percent_to_pwm(int16_t percent);

void Servo_UpdateFromPitch(void);
void Servo_UpdateFromYaw(void);
void Servo_UpdateFromRoll(void);



#endif
