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

#include "gamepad.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <tusb.h>

#define GAMEPAD_TASK_NAME "gamepad"
#define GAMEPAD_DEBOUNCE_DELAY pdMS_TO_TICKS(GAMEPAD_DEBUNCE_DELAY_MS)

#include "board_config.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Types
//--------------------------------------------------------------------+

struct device_t {
    uint pin;
    uint8_t report_bit;
};

struct device_data_t {
    const struct device_t *dev;

    bool state;
    uint8_t report_bit;

    StaticTimer_t timer_def;
    TimerHandle_t timer;
};

//--------------------------------------------------------------------+
// Variables
//--------------------------------------------------------------------+

// NOTE: The pin numbers must be sequential without any gaps
static const struct device_t g_devices[] = {
    {
        .pin  = GAMEPAD_SW1_PIN,
        .report_bit = (1U<<0),
    },
    {
        .pin  = GAMEPAD_SW2_PIN,
        .report_bit = (1U<<1),
    },
};
#define N_DEVICES (count_of(g_devices))

static_assert(GAMEPAD_SW2_PIN == GAMEPAD_SW1_PIN+1, "The pin numbers must be sequential without any gaps");



static struct device_data_t g_device_data[N_DEVICES];

#define DEVICE_DATA(gpio) (&g_device_data[gpio - g_devices[0].pin])

static uint8_t g_report = 0x00;

static TaskHandle_t g_task = NULL;


// Debounce timer callback
static void gamepad_timer_cb(TimerHandle_t xTimer)
{
    struct device_data_t *data = pvTimerGetTimerID(xTimer);
    const struct device_t *dev = data->dev;
    bool state = !gpio_get(dev->pin);
    if (state != data->state) {
        data->state = state;
        g_report = (g_report & ~dev->report_bit) | (state ? dev->report_bit : 0);

        tud_hid_report(REPORT_ID_GAMEPAD, &g_report, sizeof(g_report));
    }
}


static void gamepad_irq_cb(uint gpio, uint32_t event_mask)
{
    struct device_data_t *data = DEVICE_DATA(gpio);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerResetFromISR(data->timer, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}



void gamepad_set_report(uint8_t const* buffer, uint16_t bufsize)
{
    #if 0
    printf("Keyboard set report: bufsize=%u   data=(", bufsize);
    for (uint16_t i=0; i<bufsize; i++) {
        printf("%02x ", buffer[i]);
    }
    printf(")\n");
    #endif
}


void gamepad_init()
{
    printf("Initializing keyboard\n");

    for (uint i=0; i<N_DEVICES; i++) {
        const struct device_t *dev = &g_devices[i];
        struct device_data_t *data = &g_device_data[i];

        data->dev = dev;
        //data->report = &g_report[i];

        gpio_init(dev->pin);
        gpio_set_dir(dev->pin, GPIO_IN);
        gpio_pull_up(dev->pin);

        data->state = !gpio_get(dev->pin);

        char timer_name[16];
        sprintf(timer_name, "key_%u", i);
        data->timer = xTimerCreateStatic(timer_name, GAMEPAD_DEBOUNCE_DELAY, pdFALSE, data, gamepad_timer_cb, &data->timer_def);

        gpio_set_irq_enabled_with_callback(dev->pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, gamepad_irq_cb);
    }
}