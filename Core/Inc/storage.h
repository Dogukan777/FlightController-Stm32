#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include <stdint.h>
#include "wpManager.h"
#include "settings.h"

uint8_t WP_SaveToFlash(void);
uint8_t WP_LoadFromFlash(void);
void    WP_EraseFlash(void);

uint8_t Settings_SaveToFlash(void);
uint8_t Settings_LoadFromFlash(void);
void    Settings_EraseFlash(void);

#endif
