#include "keyboard.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <tusb.h>

#define KEYBOARD_TASK_NAME "keyboard"

#define KEYBOARD_DEBOUNCE_DELAY pdMS_TO_TICKS(KEYBOARD_DEBUNCE_DELAY_MS)

#include "board_config.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Types
//--------------------------------------------------------------------+

struct device_t {
    uint pin;
    uint code;
};

struct device_data_t {
    const struct device_t *dev;

    bool state;
    uint8_t *report;

    StaticTimer_t timer_def;
    TimerHandle_t timer;
};

//--------------------------------------------------------------------+
// Variables
//--------------------------------------------------------------------+

// NOTE: The pin numbers must be sequential without any gaps
static const struct device_t g_devices[] = {
    {
        .pin  = KEYBOARD_SW1_PIN,
        .code = KEYBOARD_SW1_CODE,
    },
    {
        .pin  = KEYBOARD_SW2_PIN,
        .code = KEYBOARD_SW2_CODE,
    },
};
#define N_DEVICES (count_of(g_devices))

static struct device_data_t g_device_data[N_DEVICES];

#define DEVICE_DATA(gpio) (&g_device_data[gpio - g_devices[0].pin])

static uint8_t g_report[6] = { 0 };

static TaskHandle_t g_task = NULL;


// Debounce timer callback
static void keyboard_timer_cb(TimerHandle_t xTimer)
{
    struct device_data_t *data = pvTimerGetTimerID(xTimer);
    const struct device_t *dev = data->dev;
    bool state = !gpio_get(dev->pin);
    if (state != data->state) {
        data->state = state;
        *data->report = state ? dev->code : HID_KEY_NONE;
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, g_report);
    }
}


static void keyboard_irq_cb(uint gpio, uint32_t event_mask)
{
    struct device_data_t *data = DEVICE_DATA(gpio);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerResetFromISR(data->timer, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}



void keyboard_set_report(uint8_t const* buffer, uint16_t bufsize)
{
    #if 0
    printf("Keyboard set report: bufsize=%u   data=(", bufsize);
    for (uint16_t i=0; i<bufsize; i++) {
        printf("%02x ", buffer[i]);
    }
    printf(")\n");
    #endif
}


void keyboard_init()
{
    printf("Initializing keyboard\n");

    for (uint i=0; i<N_DEVICES; i++) {
        const struct device_t *dev = &g_devices[i];
        struct device_data_t *data = &g_device_data[i];

        data->dev = dev;
        data->report = &g_report[i];

        gpio_init(dev->pin);
        gpio_set_dir(dev->pin, GPIO_IN);
        gpio_pull_up(dev->pin);

        data->state = !gpio_get(dev->pin);

        char timer_name[16];
        sprintf(timer_name, "key_%u", i);
        data->timer = xTimerCreateStatic(timer_name, KEYBOARD_DEBOUNCE_DELAY, pdFALSE, data, keyboard_timer_cb, &data->timer_def);

        gpio_set_irq_enabled_with_callback(dev->pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, keyboard_irq_cb);
    }
}