#include "wpManager.h"
#include "serialManager.h"
#include <string.h>
#include <stdio.h>
#include "wpStorage.h"

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
}

static void WP_OnBegin(uint16_t expected)
{
    WP_Reset();
    wp_expected = expected;
    wp_receiving = 1;
    wp_ready = 0;
}

static void WP_OnEnd(void)
{
    wp_receiving = 0;
    wp_ready = 1;
    WP_SaveToFlash();
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
            SM_SendString("ERR_FULL\n");
            return;
        }

        Waypoint tmp;
        if (WP_ParseCsv(line, &tmp)) {
            wp_list[wp_count++] = tmp;

            if (wp_expected > 0 && wp_count >= wp_expected) {
                WP_OnEnd();
            }
        } else {
             SM_SendString("ERR_PARSE\n");
        }
        return;
    }

}
void WP_SendAllToQt(void)
{
    char out[160];

    // header
    snprintf(out, sizeof(out), "WP_BEGIN,%u\n", (unsigned)wp_count);
    SM_SendString(out);

    for (uint16_t i = 0; i < wp_count; i++) {

        // status tırnak içinde gitsin
        snprintf(out, sizeof(out),
                 "WP,%.7f,%.7f,%.2f,%.2f,%.2f,\"%s\"\n",
                 wp_list[i].lat,
                 wp_list[i].lon,
                 wp_list[i].alt,
                 wp_list[i].dist,
                 wp_list[i].radius,
                 wp_list[i].status);

        SM_SendString(out);

        osDelay(2); // UART buffer taşmasın diye küçük nefes
    }

    SM_SendString("WP_END\n");
}
uint8_t WP_IsReady(void)
{
    return wp_ready;
}
void WP_SetReady(uint8_t r)
{
    wp_ready = r;
}
