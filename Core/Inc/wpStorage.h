#ifndef INC_WPSTORAGE_H_
#define INC_WPSTORAGE_H_

#include <stdint.h>
#include "wpManager.h"

uint8_t WP_SaveToFlash(void);
uint8_t WP_LoadFromFlash(void);
void    WP_EraseFlash(void);

#endif
