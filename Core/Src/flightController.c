#include "flightController.h"

// Sensör I2C Global Pointer'ları
I2C_HandleTypeDef *pMPU9250_I2C = &hi2c1;
I2C_HandleTypeDef *pMPU6050_I2C = &hi2c2;

osThreadId_t SMTaskHandle;
osThreadId_t UploadedWPTaskHandle;
osThreadId_t ppmTaskHandle;
osThreadId_t sensorTaskHandle;
osThreadId_t sensorFailSafeTaskHandle;
osThreadId_t rpiTaskHandle;

volatile uint8_t data_stream_enabled = 0;
volatile uint32_t sensor_heartbeat = 0;
volatile uint8_t  sensor_status_flags = 0; // Bit 0: MPU6050, Bit 1: MPU9250, Bit 2: GPS

const osThreadAttr_t SMTask_attributes = {
  .name = "SMTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t UploadedWPTask_attributes = {
  .name = "UploadedWPTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t ppmTask_attributes = {
  .name = "PPMTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

const osThreadAttr_t readSensor_attributes = {
  .name = "sensorTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t sensorFailSafe_attributes = {
  .name = "sensorFailSafeTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t rpiTask_attributes = {
  .name = "RPITask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

// Fonksiyon Prototipleri
void I2C_Bus_Reset(I2C_HandleTypeDef *hi2c);
void StartPPMTask(void *argument);
void StartSMTask(void *argument);
void StartUploadedWPTask(void *argument);
void StartReadSensorTask(void *argument);
void StartSensorFailSafeTask(void *argument);
void StartRPITask(void *argument);

void flightController_Init()
{
  // MPU9250 & AK8963 Başlatma (I2C1)
  MPU9250_Init(pMPU9250_I2C);

  AK8963_Init(pMPU9250_I2C);


  // MPU6050 #1 ve #2 Başlatma (I2C2)
  MPU6050_Init(pMPU6050_I2C);


  RPI_Init(&huart2);
  SM_Init(&huart6);
  SH1106_Init();
  gpsSystem_Init(&huart1);
  controlPPM_Init();
  Settings_Init();
  Servo_Init();
  StepMotor_Init();



  ppmTaskHandle            = osThreadNew(StartPPMTask, NULL, &ppmTask_attributes);
  SMTaskHandle             = osThreadNew(StartSMTask, NULL, &SMTask_attributes);
  UploadedWPTaskHandle     = osThreadNew(StartUploadedWPTask, NULL, &UploadedWPTask_attributes);
  sensorTaskHandle         = osThreadNew(StartReadSensorTask, NULL, &readSensor_attributes);
  sensorFailSafeTaskHandle = osThreadNew(StartSensorFailSafeTask, NULL, &sensorFailSafe_attributes);
  rpiTaskHandle = osThreadNew(StartRPITask, NULL, &rpiTask_attributes);
}

void StartPPMTask(void *argument)
{
  (void)argument;
  for (;;)
  {
      controlPPM_Start();
      osDelay(5); // 100ms yerine 5ms yapıldı! (200 Hz çalışma frekansı)
  }
}

void StartSMTask(void *argument)
{
  (void)argument;
  for (;;)
  {
      SM_Start();
      osDelay(10);
  }
}

void StartUploadedWPTask(void *argument)
{
  (void)argument;
  static uint16_t showIdx = 0;
  char top[32];
  char bottom[32];

  for (;;)
  {
    if (WP_IsReady() && wp_count > 0)
    {
      if (showIdx >= wp_count) showIdx = 0;

      snprintf(top, sizeof(top), "WP %u/%u", (unsigned)(showIdx + 1), (unsigned)wp_count);
      snprintf(bottom, sizeof(bottom), "LAT:%0.7f", wp_list[showIdx].lat);

      showIdx++;
    }
    osDelay(500); // Thread çakılmasın diye eklendi
  }
}

void StartReadSensorTask(void *argument)
{
    (void)argument;

    GPS_Data d;
    MPU9250_Raw imu9250;
    AK8963_Raw mag;
    int16_t accel6D[3], gyro6D[3];
    char line[128];

    for (;;)
    {
        if (!data_stream_enabled) {
            osDelay(10);
            continue;
        }

        uint8_t current_read_ok = 0;


        uint8_t gps_has_new_data = GPSTask(&d);
        if (gps_has_new_data)
                {
                    // GPS'ten canlı veri geldi
                    snprintf(line, sizeof(line),
                             "GPS,%.7f,%.7f,%.1f,%.1f,%d,%d,%.7f\n",
                             d.lat, d.lon, d.alt, d.spd_kmh, d.fix, d.sats, d.cog_deg);
                    SM_SendString(line);

                    sensor_status_flags |= (1 << 2); // Bit 2: GPS OK
                    current_read_ok = 1;
                }
                else
                {
                    // GPS'ten anlık olarak geçerli yeni paket/veri gelmedi
                    sensor_status_flags &= ~(1 << 2); // Bit 2: GPS HATA
                    //SM_SendString("GPS_ERR\n");
                }

        // 1. MPU6050 Okuma
        if (MPU6050_Read6Axis(pMPU6050_I2C, accel6D, gyro6D))
        {
            snprintf(line, sizeof(line),
                     "MPU6050_1,%d,%d,%d,%d,%d,%d\n",
                     accel6D[0], accel6D[1], accel6D[2],
                     gyro6D[0],  gyro6D[1],  gyro6D[2]);
            SM_SendString(line);

            sensor_status_flags |= (1 << 0); // MPU6050 OK
            current_read_ok = 1;
        }
        else
        {
            sensor_status_flags &= ~(1 << 0); // MPU6050 HATA
            //SM_SendString("MPU6050_1_ERR\n");
        }

        // 2. MPU9250 Okuma
        if (MPU9250_ReadRaw(pMPU9250_I2C, &imu9250) && AK8963_ReadRaw(pMPU9250_I2C, &mag))
        {
            snprintf(line, sizeof(line),
                     "MPU9250_1,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n",
                     (long)imu9250.ax, (long)imu9250.ay, (long)imu9250.az,
                     (long)imu9250.gx, (long)imu9250.gy, (long)imu9250.gz,
                     (long)mag.mx, (long)mag.my, (long)mag.mz);

            SM_SendString(line);

            sensor_status_flags |= (1 << 1); // MPU9250 OK
            current_read_ok = 1;
        }
        else
        {
            sensor_status_flags &= ~(1 << 1); // MPU9250 HATA
            //SM_SendString("MPU9250_1_ERR\n");
        }

        // En az bir sensörden veri başarıyla okunabildiyse nabız ver
        if (current_read_ok) {
            sensor_heartbeat++;
        }

        osDelay(10);
    }
}

void StartSensorFailSafeTask(void *argument)
{
    (void)argument;
    uint32_t last_heartbeat = 0;

    for (;;)
    {
        osDelay(1000); // Her 1 saniyede bir durum kontrolü yapılır

        if (data_stream_enabled)
        {
            // SENARYO 1: Tüm veri akışı kilitlendiğinde
            if (sensor_heartbeat == last_heartbeat)
            {
                SM_SendString("WARN: Sensor Stream Stalled! Re-initializing all...\n");

                I2C_Bus_Reset(pMPU9250_I2C);
                I2C_Bus_Reset(pMPU6050_I2C);

                MPU6050_Init(pMPU6050_I2C);
                MPU9250_Init(pMPU9250_I2C);
                AK8963_Init(pMPU9250_I2C);
                gpsSystem_Init(&huart1);
            }
            // SENARYO 2: Tekil sensör / GPS koptuğunda
            else
            {
                // MPU6050 Kontrolü
                if (!(sensor_status_flags & (1 << 0)))
                {
                    //SM_SendString("WARN: MPU6050 Lost! Retrying Init...\n");
                    I2C_Bus_Reset(pMPU6050_I2C);
                    MPU6050_Init(pMPU6050_I2C);
                }

                // MPU9250 Kontrolü
                if (!(sensor_status_flags & (1 << 1)))
                {
                    //SM_SendString("WARN: MPU9250 Lost! Retrying Init...\n");
                    I2C_Bus_Reset(pMPU9250_I2C);
                    MPU9250_Init(pMPU9250_I2C);
                    AK8963_Init(pMPU9250_I2C);
                }

                // GPS Kontrolü (Bağlantı kopması, UART kilitlenmesi veya veri akışının durması)
                /*if (!(sensor_status_flags & (1 << 2)))
                {
                    SM_SendString("WARN: GPS Data Lost! Restarting UART RX...\n");
                    // UART kesmesini / GPS sürücüsünü sıfırlayıp yeniden başlatıyoruz
                    gpsSystem_Init(&huart1);
                }*/
            }

            last_heartbeat = sensor_heartbeat;
        }
    }
}

void StartRPITask(void *argument)
{
  (void)argument;
  for (;;)
  {
      RPI_ProcessCommands();
      osDelay(10); // 100 Hz tarama hızı
  }
}

void I2C_Bus_Reset(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL) return;

    HAL_I2C_DeInit(hi2c);
    osDelay(10);
    HAL_I2C_Init(hi2c);
    osDelay(10);
}
