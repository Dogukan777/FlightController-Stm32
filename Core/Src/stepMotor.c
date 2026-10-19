#include "stepMotor.h"

// STM32 için yaklaşık Mikrosaniye (us) gecikme fonksiyonu
// SysTick ve SystemCoreClock kullanarak mikrosaniye bekler
static void delay_us(uint32_t us) {
    uint32_t startTicks = SysTick->VAL;
    uint32_t ticksToWait = us * (SystemCoreClock / 1000000);
    uint32_t elapsedTicks = 0;

    while (elapsedTicks < ticksToWait) {
        uint32_t currentTicks = SysTick->VAL;
        if (currentTicks <= startTicks) {
            elapsedTicks += (startTicks - currentTicks);
        } else {
            elapsedTicks += (startTicks + (SysTick->LOAD - currentTicks));
        }
        startTicks = currentTicks;
    }
}

void StepMotor_Init(void) {
    // GPIO pinlerinin CubeMX ile OUTPUT olarak ayarlandığından emin olun.
    // Başlangıç değerlerini LOW yapalım:
    HAL_GPIO_WritePin(STEP_PORT, STEP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DIR_PORT, DIR_PIN, GPIO_PIN_RESET);
}

// Belirtilen adım sayısı, yön ve mikrosaniye gecikmesi kadar motoru sürer
void StepMotor_Step(uint16_t steps, uint8_t direction, uint16_t delay_us_val) {
    // Yön belirleme (1: Saat Yönü / HIGH, 0: Ters Yön / LOW)

	if (direction == 1){
	    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // DIR pinini High yap

		for(int i = 0; i < steps; i++)
		{
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);   // STEP pinini High yap
		    osDelay(2);                                         // 2 ms bekle
		    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // STEP pinini Low yap
		    osDelay(2);                                         // 2 ms bekle (Toplam periyot 4ms)
		}

		osDelay(delay_us_val); // Dönüş bittikten sonra 1 saniye bekle
	}else{
	     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // DIR pinini Low yap

		     for(int i = 0; i < steps; i++)
		     {
		         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
		         osDelay(2);
		         HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
		         osDelay(2);
		     }

		     osDelay(delay_us_val); // 1 saniye bekle ve başa dön
	}

}

// İstenen float açıya göre motoru döndürür (+ açı: Saat Yönü, - açı: Ters Yön)
void StepMotor_RotateAngle(float angle, uint16_t delay_us_val) {
    uint8_t direction = 1;

    // Negatif açı girildiyse yönü tersine çevir ve açıyı pozitif yap
    if (angle < 0) {
        direction = 0;
        angle = -angle;
    }

    // Açıya göre gerekli adım sayısını hesapla
    // (Açı / 360 derece) * Tur başına düşen adım sayısı
    uint16_t required_steps = (uint16_t)((angle / 360.0f) * STEPS_PER_REV);

    if (required_steps > 0) {
        StepMotor_Step(required_steps, direction, delay_us_val);
    }
}
