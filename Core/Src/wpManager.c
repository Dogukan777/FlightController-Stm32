#include "wpManager.h"
#include "serialManager.h"
#include <string.h>
#include <stdio.h>


Waypoint wp_list[MAX_WP];
volatile uint16_t wp_count = 0;
volatile uint16_t wp_expected = 0;
volatile uint8_t wp_receiving = 0;
static volatile uint8_t wp_ready = 0;

void WP_Reset(void)
{
    wp_count = 0;
    wp_expected = 0;
    wp_receiving = 0;
    wp_ready = 0;
    // wp_list temizlemek şart değil; wp_count kadarını kullanacaksın.
}

static void WP_OnBegin(uint16_t expected)
{
    WP_Reset();
    wp_expected = expected;
    wp_receiving = 1;
    wp_ready = 0;
    // İstersen Qt’ye ACK at:
    // SM_SendString("OK_BEGIN\n");
}

static void WP_OnEnd(void)
{
    wp_receiving = 0;
    wp_ready = 1;
    // İstersen Qt’ye ACK:
    // SM_SendString("OK_END\n");
}

// CSV satır parse: WP,lat,lon,alt,dist,radius,"status"
static uint8_t WP_ParseCsv(const char *line, Waypoint *out)
{
    double lat, lon, alt, dist, rad;
    char status_tmp[STATUS_LEN] = {0};

    // 1) Normal: ...,"WAYPOINT"
    int ok = sscanf(line,
                    "WP,%lf,%lf,%lf,%lf,%lf,\"%31[^\"]\"",
                    &lat, &lon, &alt, &dist, &rad, status_tmp);

    if (ok != 6) {
        // 2) Kaçışlı: ...,\"WAYPOINT\"
        ok = sscanf(line,
                    "WP,%lf,%lf,%lf,%lf,%lf,\\\"%31[^\\\"]\\\"",
                    &lat, &lon, &alt, &dist, &rad, status_tmp);
    }

    if (ok != 6) return 0;

    out->lat = lat;
    out->lon = lon;
    out->alt = alt;
    out->dist = dist;
    out->radius = rad;

    strncpy(out->status, status_tmp, STATUS_LEN - 1);
    out->status[STATUS_LEN - 1] = '\0';
    return 1;
}


void WP_ProcessLine(const char *line)
{
    if (!line || line[0] == '\0') return;

    // 1) WP_BEGIN,<N>
    if (strncmp(line, "WP_BEGIN,", 9) == 0) {
        int n = 0;
        if (sscanf(line, "WP_BEGIN,%d", &n) == 1) {
            if (n < 0) n = 0;
            if (n > MAX_WP) n = MAX_WP;
            WP_OnBegin((uint16_t)n);
        }
        return;
    }

    // 2) WP_END
    if (strcmp(line, "WP_END") == 0) {
        WP_OnEnd();
        return;
    }

    // 3) WP,... satırları
    if (wp_receiving && strncmp(line, "WP,", 3) == 0) {
        if (wp_count >= MAX_WP) {
            // taşma, alımı durdurmak istersen:
            // wp_receiving = 0;
            SM_SendString("ERR_FULL\n");
            return;
        }

        Waypoint tmp;
        if (WP_ParseCsv(line, &tmp)) {
            wp_list[wp_count++] = tmp;

            // beklenen sayı kadar geldiyse otomatik bitir (opsiyonel)
            if (wp_expected > 0 && wp_count >= wp_expected) {
                WP_OnEnd();
            }
        } else {
            // parse hatası
             SM_SendString("ERR_PARSE\n");
        }
        return;
    }

    // Diğer mesajlar (CONNECT/DISCONNECT vs) burada ignore edilebilir.
}

uint8_t WP_IsReady(void)
{
    return wp_ready;
}
