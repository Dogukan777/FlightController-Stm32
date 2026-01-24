#ifndef INC_WPMANAGER_H_
#define INC_WPMANAGER_H_

#include <stdint.h>

#define MAX_WP 100
#define STATUS_LEN 32

typedef struct {
    double lat;
    double lon;
    double alt;
    double dist;
    double radius;
    char status[STATUS_LEN];
} Waypoint;

extern Waypoint wp_list[MAX_WP];
extern volatile uint16_t wp_count;
extern volatile uint16_t wp_expected;
extern volatile uint8_t  wp_receiving;

void WP_Reset(void);
void WP_ProcessLine(const char *line);
uint8_t WP_IsReady(void);
void WP_SetReady(uint8_t r);
void WP_SendAllToQt(void);



#endif
