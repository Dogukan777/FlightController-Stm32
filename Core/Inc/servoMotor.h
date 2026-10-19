#ifndef INC_SERVOMOTOR_H_
#define INC_SERVOMOTOR_H_

#include "main.h"

// --- Servo Parametreleri ---
// Eğer servonuz 270 derece ise 270.0f yazın
#define SERVO_MAX_ANGLE     180.0f

// Servo Darbe Genişliği Sınırları (Microseconds - us cinsinden)
// DSServo dijital servolar için varsayılan değerler:
#define SERVO_MIN_PULSE_US  500    // 0 derece için 500us
#define SERVO_MAX_PULSE_US  2500   // Maksimum derece için 2500us

// --- Fonksiyon Prototipleri ---

/**
 * @brief Servo motor PWM sinyalini (TIM4 Channel 3) başlatır.
 */
void Servo_Init(void);

/**
 * @brief Servoyu doğrudan microsecond (us) cinsinden sinyal genişliği ile sürer.
 * @param us_pulse: 500 - 2500 arası pulse değeri
 */
void Servo_WriteMicroseconds(uint16_t us_pulse);

/**
 * @brief Servoyu verilen derece açısına döndürür.
 * @param angle: 0 ile SERVO_MAX_ANGLE arası derece değeri (Örn: 0.0 - 180.0)
 */
void Servo_WriteAngle(float angle);

/**
 * @brief Servoyu yumuşak (kademeli) bir şekilde hedef açıya döndürür.
 * @param target_angle: Hedef açı
 * @param step_delay_ms: Adımlar arası bekleme süresi (Hız kontrolü)
 */
void Servo_WriteAngleSmooth(float current_angle, float target_angle, uint32_t step_delay_ms);

#endif /* INC_SERVOMOTOR_H_ */
