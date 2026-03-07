#include "gpsSystem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ====== INTERNALS ======
static UART_HandleTypeDef *gps_huart = NULL;
static volatile uint8_t gps_rx_ch;

static char nmea_line[128];
static volatile uint16_t nmea_idx = 0;
static volatile uint8_t  nmea_ready = 0;
static volatile uint8_t  nmea_sync  = 0;
static char nmea_last[128];

// paylaşılan GPS state (task okuyacak)
static volatile double g_lat = 0.0, g_lon = 0.0, g_alt = 0.0, g_spd = 0.0;
static volatile double g_cog = 0.0;
static volatile int    g_fix = 0,   g_sats = 0;
static volatile char   g_rmc_status = 'V';

// ---- helpers ----
static int nmea_get_field(const char* s, int field_index, char* out, int out_sz)
{
  int idx = 0;
  const char* p = s;
  const char* start = s;

  while (*p && idx < field_index) {
    if (*p == ',') { idx++; start = p + 1; }
    p++;
  }
  if (idx != field_index) return 0;

  int n = 0;
  while (start[n] && start[n] != ',' && start[n] != '*' && n < out_sz - 1) n++;
  memcpy(out, start, n);
  out[n] = 0;
  return 1;
}

static int nmea_degmin_to_dec(const char* dm, char hemi, double* out_deg)
{
  if (!dm || !dm[0]) return 0;
  double v = atof(dm);
  int deg = (int)(v / 100.0);
  double min = v - (deg * 100.0);
  double dec = (double)deg + (min / 60.0);
  if (hemi == 'S' || hemi == 'W') dec = -dec;
  *out_deg = dec;
  return 1;
}

static int parse_rmc(const char* line,
                     double* lat, double* lon,
                     double* speed_kmh,
                     double* cog_deg,
                     char* status)
{
  char st[4], lat_s[20], ns[4], lon_s[20], ew[4], spd_s[16], cog_s[16];

  if (!nmea_get_field(line, 2, st, sizeof(st))) return 0;
  *status = st[0];

  if (*status != 'A') {
    *lat = 0.0; *lon = 0.0;
    *speed_kmh = 0.0;
    *cog_deg = 0.0;
    return 1;
  }

  nmea_get_field(line, 3, lat_s, sizeof(lat_s));
  nmea_get_field(line, 4, ns, sizeof(ns));
  nmea_get_field(line, 5, lon_s, sizeof(lon_s));
  nmea_get_field(line, 6, ew, sizeof(ew));

  nmea_get_field(line, 7, spd_s, sizeof(spd_s)); // knots
  nmea_get_field(line, 8, cog_s, sizeof(cog_s)); // course over ground (deg)

  nmea_degmin_to_dec(lat_s, ns[0], lat);
  nmea_degmin_to_dec(lon_s, ew[0], lon);

  double spd_knots = atof(spd_s);
  *speed_kmh = spd_knots * 1.852;

  *cog_deg = (cog_s[0] ? atof(cog_s) : 0.0);
  return 1;
}

static int parse_gga(const char* line, double* lat, double* lon, double* alt_m, int* fix, int* sats)
{
  char lat_s[20], ns[4], lon_s[20], ew[4], fix_s[8], sat_s[8], alt_s[16];

  if (!nmea_get_field(line, 6, fix_s, sizeof(fix_s))) return 0;
  if (!nmea_get_field(line, 7, sat_s, sizeof(sat_s))) return 0;

  *fix  = atoi(fix_s);
  *sats = atoi(sat_s);

  if (nmea_get_field(line, 9, alt_s, sizeof(alt_s))) *alt_m = atof(alt_s);
  else *alt_m = 0.0;

  if (*fix == 0) { *lat = 0.0; *lon = 0.0; return 1; }

  if (!nmea_get_field(line, 2, lat_s, sizeof(lat_s))) return 0;
  if (!nmea_get_field(line, 3, ns, sizeof(ns))) return 0;
  if (!nmea_get_field(line, 4, lon_s, sizeof(lon_s))) return 0;
  if (!nmea_get_field(line, 5, ew, sizeof(ew))) return 0;

  if (!lat_s[0] || !lon_s[0]) return 1;

  if (!nmea_degmin_to_dec(lat_s, ns[0], lat)) return 0;
  if (!nmea_degmin_to_dec(lon_s, ew[0], lon)) return 0;

  return 1;
}

// ====== PUBLIC ======
void gpsSystem_Init(UART_HandleTypeDef *huart)
{
  gps_huart = huart;
  nmea_idx = 0;
  nmea_ready = 0;
  nmea_sync = 0;

  // 1 byte RX interrupt start
  HAL_UART_Receive_IT(gps_huart, (uint8_t*)&gps_rx_ch, 1);
}

// Bu fonksiyon UART callback içinden çağrılacak
void gpsSystem_OnRxByte(uint8_t b)
{
  char c = (char)b;

  if (!nmea_ready)
  {
    if (!nmea_sync)
    {
      if (c == '$') {
        nmea_sync = 1;
        nmea_idx = 0;
        nmea_line[nmea_idx++] = c;
      }
    }
    else
    {
      if (c == '\n') {
        nmea_line[nmea_idx] = 0;
        strncpy(nmea_last, nmea_line, sizeof(nmea_last));
        nmea_last[sizeof(nmea_last)-1] = 0;
        nmea_ready = 1;
        nmea_sync = 0;
        nmea_idx = 0;
      }
      else if (c != '\r') {
        if (nmea_idx < sizeof(nmea_line)-1) nmea_line[nmea_idx++] = c;
        else { nmea_sync = 0; nmea_idx = 0; }
      }
    }
  }
}
int parse_vtg(const char* line, double* cog_deg, double* spd_kmh)
{
  char cog_s[16], spd_k_s[16];

  // field 1: course (true)
  // field 7: speed (km/h)
  if (!nmea_get_field(line, 1, cog_s, sizeof(cog_s))) return 0;
  if (!nmea_get_field(line, 7, spd_k_s, sizeof(spd_k_s))) return 0;

  if (cog_s[0]) *cog_deg = atof(cog_s);
  if (spd_k_s[0]) *spd_kmh = atof(spd_k_s);

  return 1;
}

// Task içinde sürekli çağır: “hazır satır” varsa parse eder
void gpsSystem_TaskStep(void)
{
  if (!nmea_ready) return;

  if (!strncmp(nmea_last, "$GNGGA", 6) || !strncmp(nmea_last, "$GPGGA", 6))
  {
    double lat=0.0, lon=0.0, alt=0.0;
    int fix=0, sats=0;

    if (parse_gga(nmea_last, &lat, &lon, &alt, &fix, &sats))
    {
      g_fix  = fix;
      g_sats = sats;
      if (fix != 0) {
        g_lat = lat; g_lon = lon; g_alt = alt;
      }
    }
  }
  else if (!strncmp(nmea_last, "$GNRMC", 6) || !strncmp(nmea_last, "$GPRMC", 6))
  {
    double lat=0.0, lon=0.0, spd=0.0, cog=0.0;
    char st='V';

    if (parse_rmc(nmea_last, &lat, &lon, &spd, &cog, &st))
    {
      g_rmc_status = st;
      g_spd = spd;
      g_cog = cog;
      if (st == 'A') { g_lat = lat; g_lon = lon; }
    }
  }
  else if (!strncmp(nmea_last, "$GNVTG", 6) || !strncmp(nmea_last, "$GPVTG", 6))
  {
    double cog=0.0, spd=0.0;
    if (parse_vtg(nmea_last, &cog, &spd)) {
      g_cog = cog;
      g_spd = spd; // istersen
    }
  }

  nmea_ready = 0;
}

void gpsSystem_Get(GPS_Data *out)
{
  if (!out) return;

  uint32_t prim = __get_PRIMASK();
  __disable_irq();

  out->lat = g_lat;
  out->lon = g_lon;
  out->alt = g_alt;
  out->spd_kmh = g_spd;
  out->cog_deg = g_cog;
  out->fix = g_fix;
  out->sats = g_sats;
  out->rmc_status = g_rmc_status;

  if (!prim) __enable_irq();
}

void gpsSystem_OnUartRxCplt(UART_HandleTypeDef *huart)
{
  if (gps_huart && huart->Instance == gps_huart->Instance) {
    gpsSystem_OnRxByte(gps_rx_ch);
    HAL_UART_Receive_IT(gps_huart, (uint8_t*)&gps_rx_ch, 1);
  }
}
