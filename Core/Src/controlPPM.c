// Fonksiyon Prototipleri
void Brushed_Motor_Write(uint16_t us);
void Dual_Motor_Write(uint16_t throttle_us, uint16_t steering_us);
void Motor_Set_PWM(TIM_HandleTypeDef *htim, int32_t speed);
static void Set_Motor_Outputs(int32_t left_duty, int32_t right_duty, uint32_t arr);
/**
 * @brief FreeRTOS Task'ı Tarafından Sürekli Çağrılan Fonksiyon
 */
void controlPPM_Start(void)
{
    uint32_t now = HAL_GetTick();
    static uint32_t last_print_ms = 0;
    char debug_msg[128];

    // FAILSAFE: 200ms boyunca geçerli PPM sinyali gelmezse motoru durdur ve bildir!
    if (now - ppm_last_ms > 200) {
        Dual_Motor_Write(1500, 1500); // Güvenli duruş için her iki kanalı nötr yap

        if (now - last_print_ms >= 500) {
            last_print_ms = now;
            SM_SendString("PPM_ERR: Signal Lost!\n");
        }
        return;
    }

    // CH1: İleri/Geri (Throttle), CH0: Sağa/Sola Dönüş (Steering)
    uint16_t throttle_us = ppm_ch[1];
    uint16_t steering_us = ppm_ch[0];

    // --- UART TELEMETRİ GÖNDERİMİ (Her 100ms'de bir) ---
    if (now - last_print_ms >= 100) {
        last_print_ms = now;

        snprintf(debug_msg, sizeof(debug_msg),
                 "PPM_RAW -> CH0(Steer):%u | CH1(Throt):%u\n",
                 ppm_ch[0], ppm_ch[1]);

        SM_SendString(debug_msg);
    }

    // Çift motor diferansiyel sürüş fonksiyonunu çağır
    Dual_Motor_Write(throttle_us, steering_us);
}

/**
 * @brief Sağ ve Sol Motorlar için PWM Değerlerini Ayarlayan Düşük Seviyeli Fonksiyona Yardımcı
 */
static void Set_Motor_Outputs(int32_t left_duty, int32_t right_duty, uint32_t arr)
{
    // SOL MOTOR (TIM2) Kontrolü
    if (left_duty > 0) {
        if (left_duty > (int32_t)arr) left_duty = arr;
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (uint32_t)left_duty); // Sol İleri
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    } else if (left_duty < 0) {
        int32_t duty_abs = -left_duty;
        if (duty_abs > (int32_t)arr) duty_abs = arr;
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)duty_abs); // Sol Geri
    } else {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    }

    // SAĞ MOTOR (TIM3) Kontrolü
    if (right_duty > 0) {
        if (right_duty > (int32_t)arr) right_duty = arr;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)right_duty); // Sağ İleri
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    } else if (right_duty < 0) {
        int32_t duty_abs = -right_duty;
        if (duty_abs > (int32_t)arr) duty_abs = arr;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, (uint32_t)duty_abs); // Sağ Geri
    } else {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    }
}

/**
 * @brief Throttle (CH1) ve Steering (CH0) değerlerini harmanlayarak Diferansiyel Sürüş sağlar
 */
void Dual_Motor_Write(uint16_t throttle_us, uint16_t steering_us)
{
    const uint16_t deadband = 30; // Ölü bölge
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);

    // Sınırlandırmalar
    if (throttle_us < 1000) throttle_us = 1000;
    if (throttle_us > 2000) throttle_us = 2000;
    if (steering_us < 1000) steering_us = 1000;
    if (steering_us > 2000) steering_us = 2000;

    // 1500 merkez alınarak -500 ile +500 arasında normalize etme
    int32_t t_val = (int32_t)throttle_us - 1500;
    int32_t s_val = (int32_t)steering_us - 1500;

    // Ölü bölge filtreleri
    if (t_val > -deadband && t_val < deadband) t_val = 0;
    if (s_val > -deadband && s_val < deadband) s_val = 0;

    // Diferansiyel Karıştırıcı (Mixer) Algoritması
    // İleri/Geri gücüne sağa/sola dönüş etkisini ekleyip çıkarıyoruz
    int32_t left_raw  = t_val + s_val;
    int32_t right_raw = t_val - s_val;

    // PWM Duty oranına dönüştürme (500us maksimum sapma aralığı üzerinden oranlama)
    int32_t left_duty  = (left_raw * (int32_t)arr) / (500 - deadband);
    int32_t right_duty = (right_raw * (int32_t)arr) / (500 - deadband);

    // Çıkışları uygula
    Set_Motor_Outputs(left_duty, right_duty, arr);
}
