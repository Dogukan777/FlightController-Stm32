#include <storage.h>
#include "stm32f4xx_hal.h"
#include <string.h>

#define WP_FLASH_SECTOR   FLASH_SECTOR_7
#define WP_FLASH_ADDR     0x08060000U
#define WP_MAGIC          0x57504C54u // 'WPLT'
#define WP_MAX_SAVE       MAX_WP

typedef struct {
  uint32_t magic;
  uint16_t count;
  uint16_t reserved;
} WpHeader;

typedef struct {
  int32_t lat_e7;
  int32_t lon_e7;
  int32_t alt_cm;
  int32_t dist_cm;
  int32_t rad_cm;
  char status[STATUS_LEN];
  uint8_t pad[ (4 - (STATUS_LEN % 4)) % 4 ];
} WpPacked;

static uint32_t sector_to_addr(uint32_t sector)
{
  // F411 sector base addresses (512KB):
  // S0 0x08000000 16K
  // S1 0x08004000 16K
  // S2 0x08008000 16K
  // S3 0x0800C000 16K
  // S4 0x08010000 64K
  // S5 0x08020000 128K
  // S6 0x08040000 128K
  // S7 0x08060000 128K
  switch (sector) {
    case FLASH_SECTOR_0: return 0x08000000U;
    case FLASH_SECTOR_1: return 0x08004000U;
    case FLASH_SECTOR_2: return 0x08008000U;
    case FLASH_SECTOR_3: return 0x0800C000U;
    case FLASH_SECTOR_4: return 0x08010000U;
    case FLASH_SECTOR_5: return 0x08020000U;
    case FLASH_SECTOR_6: return 0x08040000U;
    case FLASH_SECTOR_7: return 0x08060000U;
    default: return 0x08060000U;
  }
}

void WP_EraseFlash(void)
{
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase = {0};
  uint32_t sectorError = 0;

  erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase.Sector       = WP_FLASH_SECTOR;
  erase.NbSectors    = 1;

  HAL_FLASHEx_Erase(&erase, &sectorError);

  HAL_FLASH_Lock();
}

uint8_t WP_SaveToFlash(void)
{
  if (wp_count > WP_MAX_SAVE) return 0;

  // 1) Her durumda flash'ı temizle (overwrite garantisi)
  WP_EraseFlash();

  // 2) Header hazırla
  WpHeader hdr;
  hdr.magic = WP_MAGIC;
  hdr.count = (uint16_t)wp_count;
  hdr.reserved = 0;

  // 3) Yaz
  HAL_FLASH_Unlock();

  uint32_t addr = WP_FLASH_ADDR;

  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, hdr.magic) != HAL_OK) goto fail;
  addr += 4;

  uint32_t secondWord = ((uint32_t)hdr.count) | (((uint32_t)hdr.reserved) << 16);
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, secondWord) != HAL_OK) goto fail;
  addr += 4;

  // Eğer wp_count == 0 ise burada bitir: eski kayıtlar artık yok
  if (hdr.count == 0) {
    HAL_FLASH_Lock();
    return 1;
  }

  // 4) Waypointleri yaz
  for (uint16_t i = 0; i < hdr.count; i++)
  {
    WpPacked p;
    p.lat_e7  = (int32_t)(wp_list[i].lat * 10000000.0);
    p.lon_e7  = (int32_t)(wp_list[i].lon * 10000000.0);
    p.alt_cm  = (int32_t)(wp_list[i].alt * 100.0);
    p.dist_cm = (int32_t)(wp_list[i].dist * 100.0);
    p.rad_cm  = (int32_t)(wp_list[i].radius * 100.0);
    memset(p.status, 0, STATUS_LEN);
    strncpy(p.status, wp_list[i].status, STATUS_LEN - 1);

    const uint32_t *w = (const uint32_t*)&p;
    for (uint32_t k = 0; k < sizeof(WpPacked)/4; k++) {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, w[k]) != HAL_OK) goto fail;
      addr += 4;
    }
  }

  HAL_FLASH_Lock();
  return 1;

fail:
  HAL_FLASH_Lock();
  return 0;
}

uint8_t WP_LoadFromFlash(void)
{
  const uint32_t *mem = (const uint32_t*)WP_FLASH_ADDR;

  uint32_t magic = mem[0];
  if (magic != WP_MAGIC) return 0;

  uint32_t word1 = mem[1];
  uint16_t count = (uint16_t)(word1 & 0xFFFFu);
  if (count == 0 || count > WP_MAX_SAVE) return 0;

  // ok, doldur
  WP_Reset();
  wp_expected = count;
  wp_count = count;

  const uint8_t *ptr = (const uint8_t*)(WP_FLASH_ADDR + 8);

  for (uint16_t i = 0; i < count; i++)
  {
    WpPacked p;
    memcpy(&p, ptr, sizeof(WpPacked));
    ptr += sizeof(WpPacked);

    wp_list[i].lat    = ((double)p.lat_e7)  / 10000000.0;
    wp_list[i].lon    = ((double)p.lon_e7)  / 10000000.0;
    wp_list[i].alt    = ((double)p.alt_cm)  / 100.0;
    wp_list[i].dist   = ((double)p.dist_cm) / 100.0;
    wp_list[i].radius = ((double)p.rad_cm)  / 100.0;
    strncpy(wp_list[i].status, p.status, STATUS_LEN-1);
    wp_list[i].status[STATUS_LEN-1] = '\0';
  }

  // hazır
  // wp_ready wpManager.c içinde static; onu setlemek için fonksiyon ekleyelim:
  // WP_SetReady(1);
  return 1;
}
