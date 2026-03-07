#include "flightController.h"
#include "controlPPM.h"
#include "gpsSystem.h"

osThreadId_t SMTaskHandle;
osThreadId_t UploadedWPTaskHandle;
osThreadId_t gyroTaskHandle;
osThreadId_t ppmTaskHandle;
osThreadId_t gpsTaskHandle;
volatile uint8_t data_stream_enabled = 0;

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
const osThreadAttr_t gyroTask_attributes = {
  .name = "gyroTask",
  .stack_size = 256 * 4,                 // 1KB
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t ppmTask_attributes = {
  .name = "PPMTask",
  .stack_size = 256 * 4,                 // 1KB yeter
  .priority = (osPriority_t) osPriorityHigh, // ESC kontrol daha öncelikli olsun
};
const osThreadAttr_t gpsTask_attributes = {
  .name = "GPSTask",
  .stack_size = 256 * 4,                 // 1KB yeter
  .priority = (osPriority_t) osPriorityHigh, // ESC kontrol daha öncelikli olsun
};

void flightController_Init()
{
  GYRO_Init(&hi2c2);
  SM_Init(&huart6);
  SH1106_Init();
  gpsSystem_Init(&huart1);
  controlPPM_Init();

  //displayTwoLines("MErhaba","Dünya");
  ppmTaskHandle = osThreadNew(StartPPMTask, NULL, &ppmTask_attributes);
  SMTaskHandle         = osThreadNew(StartSMTask, NULL, &SMTask_attributes);
  UploadedWPTaskHandle = osThreadNew(StartUploadedWPTask, NULL, &UploadedWPTask_attributes);
  gyroTaskHandle       = osThreadNew(StartGyroTask, NULL, &gyroTask_attributes);

}


void displayTwoLines(const char *top, const char *bottom)
{
  SH1106_Clear();

  // Üst satır
  SH1106_GotoXY(5, 5);
  SH1106_Puts((char*)top, &Font_7x10, 1);

  // Alt satır
  SH1106_GotoXY(5, 30);
  SH1106_Puts((char*)bottom, &Font_7x10, 1);

  SH1106_UpdateScreen();

  if (osKernelGetState() == osKernelRunning) osDelay(5);
  else HAL_Delay(5);
}

void GPSTask(GPS_Data *d)
{
    gpsSystem_TaskStep();
    gpsSystem_Get(d);

    if (SM_IsConnected() && gps_stream_enabled) {
        char msg[128];

      snprintf(msg, sizeof(msg),
            "GPS,%.7f,%.7f,%.1f,%.1f,%d,%d,%.7f\n",
            d->lat, d->lon, d->alt, d->spd_kmh, d->fix, d->sats,d->cog_deg);
        SM_SendString(msg);
    }
}




void StartPPMTask(void *argument)
{
  (void)argument;


  for (;;)
  {
	  controlPPM_Start();
	  osDelay(5);
  }
}




void StartSMTask(void *argument)
{
  (void)argument;

  GPS_Data d;

  for (;;)
  {
	  GPSTask(&d);
	  SM_Start();
	  osDelay(5);
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

      // Üst: WP sayısı ve index
      snprintf(top, sizeof(top),
               "WP %u/%u",
               (unsigned)(showIdx + 1),
               (unsigned)wp_count);

      // Alt: LAT
      snprintf(bottom, sizeof(bottom),
               "LAT:%0.7f",
               wp_list[showIdx].lat);

      //displayTwoLines(top, bottom);

      showIdx++;
      //osDelay(1000);
      //continue;
    }

  }
}

void StartGyroTask(void *argument)
{
    (void)argument;

    MPU6050_Raw raw;
    char line[96];

    for (;;)
    {
        if (!data_stream_enabled) {
            osDelay(10);
            continue;
        }

        if (GYRO_ReadRaw(&hi2c2, &raw))
        {
            int n = snprintf(line, sizeof(line),
                             "DATA,%d,%d,%d,%d,%d,%d\n",
                             (int)raw.ax, (int)raw.ay, (int)raw.az,
                             (int)raw.gx, (int)raw.gy, (int)raw.gz);

            if (n > 0) {
                SM_SendString(line);
            }
        }
        else
        {
            SM_SendString("DATA_ERR\n");
        }
    }
}




