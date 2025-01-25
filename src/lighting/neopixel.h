/**
 * BSD 3-Clause License
 * 
 * Copyright (c) 2025, Peter Christoffersen
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
uint32_t neopixel_strip_update_latency_us(uint id);
uint32_t neopixel_strip_transmit_time_us(uint id);