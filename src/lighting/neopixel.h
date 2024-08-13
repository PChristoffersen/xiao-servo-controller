#pragma once

#include <pico/stdlib.h>


typedef uint32_t color_t;

void neopixel_init();

enum neopixel_strip_id {
    NEOPIXEL_STRIP0,
    NEOPIXEL_STRIP1,
};



static inline color_t ugrb_color(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((color_t) (g) << 24) |
            ((color_t) (r) << 16) |
            ((color_t) (b) << 8);
}

static inline color_t ugrbw_color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return
            ((color_t) (g) << 24) |
            ((color_t) (r) << 16) |
            ((color_t) (b) << 8);
            ((color_t) (w));
}



#define NEOPIXEL_BLACK 0x00000000

void neopixel_strip_set_enabled(uint id, bool enabled);
bool neopixel_strip_get_enabled(uint id);
void neopixel_strip_fill(uint id, color_t color);
void neopixel_strip_set(uint id, uint idx, color_t color);
void neopixel_strip_show(uint id);
