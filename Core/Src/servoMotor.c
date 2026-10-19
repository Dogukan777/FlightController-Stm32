#include "servoMotor.h"

// main.c içinde tanımlı olan TIM4 handle yapısını projeden çekiyoruz
extern TIM_HandleTypeDef htim4;

/**
 * @brief TIM4 Channel 3 PWM Çıkışını Başlatır
 */
void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    // Açılışta servoyu orta konuma (90 Derece / 1500us) getir
    Servo_WriteAngle(SERVO_MAX_ANGLE / 2.0f);
}

/**
 * @brief Doğrudan Microsecond (us) değerini TIM4 CH3 Compare register'ına yazar.
 */
void Servo_WriteMicroseconds(uint16_t us_pulse)
{
    // Güvenlik için pulse sınırlandırması (Limit)
    if (us_pulse < SERVO_MIN_PULSE_US) us_pulse = SERVO_MIN_PULSE_US;
    if (us_pulse > SERVO_MAX_PULSE_US) us_pulse = SERVO_MAX_PULSE_US;

    // ARR register çözünürlüğü 1us olacak şekilde ayarlandığından
    // CCR değeri doğrudan us değerine eşittir.
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, us_pulse);
}

/**
 * @brief Dereceyi (0° - 180°/270°) pulse genişliğine dönüştürüp servoyu döndürür.
 */
void Servo_WriteAngle(float angle)
{
    // Açı sınırlandırması
    if (angle < 0.0f) angle = 0.0f;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    // Lineer İnterpolasyon (Dereceyi -> Pulse Genişliğine Çevirme)
    // Formula: pulse = Min + (angle / MaxAngle) * (MaxPulse - MinPulse)
    uint16_t us_pulse = (uint16_t)(SERVO_MIN_PULSE_US +
                        ((angle / SERVO_MAX_ANGLE) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)));

    Servo_WriteMicroseconds(us_pulse);
}

/**
 * @brief Servonun aniden sarsılarak dönmesini önlemek için kademeli geçiş sağlar.
 */
void Servo_WriteAngleSmooth(float current_angle, float target_angle, uint32_t step_delay_ms)
{
    if (target_angle > SERVO_MAX_ANGLE) target_angle = SERVO_MAX_ANGLE;
    if (target_angle < 0.0f) target_angle = 0.0f;

    if (current_angle < target_angle)
    {
        for (float a = current_angle; a <= target_angle; a += 1.0f)
        {
            Servo_WriteAngle(a);
            HAL_Delay(step_delay_ms);
        }
    }
    else
    {
        for (float a = current_angle; a >= target_angle; a -= 1.0f)
        {
            Servo_WriteAngle(a);
            HAL_Delay(step_delay_ms);
        }
    }
}
