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
