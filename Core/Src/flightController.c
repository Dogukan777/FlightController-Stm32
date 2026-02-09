#include "flightController.h"
#include "controlPPM.h"

osThreadId_t SMTaskHandle;
osThreadId_t UploadedWPTaskHandle;
osThreadId_t gyroTaskHandle;
osThreadId_t ppmTaskHandle;
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

void flightController_Init()
{
  GYRO_Init(&hi2c2);
  SM_Init(&huart2);
  SH1106_Init();

  controlPPM_Init();


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

  char line[128];

  for (;;)
  {
    while (SM_ReadLine(line, sizeof(line)) > 0)
    {
      if (strcmp(line, "CONNECT") == 0)
      {
        if (sm_connected == 0) {
          sm_connected = 1;
          displayScreen("Connect");
          SM_SendString("TRUE\n");
        }
      }
      else if (strcmp(line, "DISCONNECT") == 0)
      {
        if (sm_connected == 1) {
          sm_connected = 0;
          displayScreen("Disconnect");
          SM_SendString("FALSE\n");
        }
      }
      else if (strcmp(line, "READ") == 0) {
             if (WP_LoadFromFlash()) {
            	 WP_SetReady(1);
            	 WP_SendAllToQt();
             } else {
            	 SM_SendString("WP_BEGIN,0\nWP_END\n");
             }
      }
      else if (strcmp(line, "DATA") == 0)
      {
          data_stream_enabled = 1;
          SM_SendString("DATA_BEGIN\n");
      }
      else if (strcmp(line, "DATA_STOP") == 0)
      {
          data_stream_enabled = 0;
          SM_SendString("DATA_END\n");
      }
      else {
             // WP upload satırları
             WP_ProcessLine(line);
      }
    }

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
      osDelay(2000);
      continue;
    }

    osDelay(50);
  }
}

void StartGyroTask(void *argument)
{
  (void)argument;

  MPU6050_Raw raw;
  char line[96];

  const uint32_t period_ms = 10;
  uint32_t last = osKernelGetTickCount();

  for (;;)
  {
    if (!data_stream_enabled) {
      osDelay(20);
      continue;
    }

    if (GYRO_ReadRaw(&hi2c2, &raw))
    {
      // CSV format: DATA,ax,ay,az,gx,gy,gz
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

    uint32_t now = osKernelGetTickCount();
    uint32_t elapsed = now - last;
    if (elapsed < period_ms) osDelay(period_ms - elapsed);
    last = osKernelGetTickCount();
  }
}





