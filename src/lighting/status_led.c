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

#include "status_led.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "board_config.h"


#if STATUS_LED_ACTIVE_LOW
#define LED_ON false
#define LED_OFF true
#else
#define LED_ON true
#define LED_OFF false
#endif

#define RED_MASK   (1<<0)
#define GREEN_MASK (1<<1)
#define BLUE_MASK  (1<<2)

static inline void led_init(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, LED_OFF);
}


void status_led_init()
{
    led_init(STATUS_LED_R_PIN);
    led_init(STATUS_LED_G_PIN);
    led_init(STATUS_LED_B_PIN);
}


void status_led_put_raw(bool red, bool green, bool blue)
{
    gpio_put(STATUS_LED_R_PIN, red ? LED_ON : LED_OFF);
    gpio_put(STATUS_LED_G_PIN, green ? LED_ON : LED_OFF);
    gpio_put(STATUS_LED_B_PIN, blue ? LED_ON : LED_OFF);
}



void status_led_put(status_color_t col) 
{
    status_led_put_raw((col&RED_MASK), (col&GREEN_MASK), (col&BLUE_MASK));
}


void status_neopixel_put(color_t color)
{
    neopixel_strip_set(STATUS_NEOPIXEL_LED, 0, color);
    neopixel_strip_show(STATUS_NEOPIXEL_LED);
}
