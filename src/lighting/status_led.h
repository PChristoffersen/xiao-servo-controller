#pragma once

#include <pico/stdlib.h>
#include "neopixel.h"

typedef enum {
    STATUS_COL_BLACK   = 0b000,
    STATUS_COL_RED     = 0b001,
    STATUS_COL_GREEN   = 0b010,
    STATUS_COL_YELLOW  = 0b011,
    STATUS_COL_BLUE    = 0b100,
    STATUS_COL_MAGENTA = 0b101,
    STATUS_COL_CYAN    = 0b110,
    STATUS_COL_WHITE   = 0b110,
} status_color_t;

void status_led_init();


void status_led_put(status_color_t col);
void status_led_put_raw(bool red, bool green, bool blue);

void status_neopixel_put(color_t color);
