#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/watchdog.h>
#include <FreeRTOS.h>
#include <task.h>

#include "board_config.h"
#include "hid/keyboard.h"
#include "lighting/status_led.h"
#include "lighting/neopixel.h"
#include "uart/pio_uart.h"

static_assert(USBD_TASK_PRIORITY < configMAX_PRIORITIES);
#define USBD_TASK_NAME "usbd"
#define UART_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define STATUS_TASK_NAME "status"
#define STATUS_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

static bool g_usb_device_configured = false;


#define STATISTICS_TASK_PRIORITY ( 1 )
#define TEST_TASK_PRIORITY       ( 11 )




#if 0
static void test_task(__unused void *params)
{
    uint cnt = 0;

    neopixel_strip_set_enabled(HID_NEOPIXEL_STRIP, true);
    neopixel_strip_fill(HID_NEOPIXEL_STRIP, NEOPIXEL_BLACK);
    neopixel_strip_show(HID_NEOPIXEL_STRIP);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
        neopixel_strip_set(HID_NEOPIXEL_STRIP, cnt, ugrb_color(0x00, 0x00, 0x20));
        neopixel_strip_show(HID_NEOPIXEL_STRIP);

        cnt++;
        if (cnt>=HID_NEOPIXEL_COUNT) {
            neopixel_strip_fill(HID_NEOPIXEL_STRIP, NEOPIXEL_BLACK);
            cnt = 0;
        }
    }

    vTaskDelete(NULL);
}
#endif



//--------------------------------------------------------------------+
// Init
//--------------------------------------------------------------------+

static void board_init()
{
    stdio_init_all();

    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
    }
    watchdog_enable(WATCHDOG_INTERVAL_MS, true);

    status_led_init();
    neopixel_init();
}

//--------------------------------------------------------------------+
// Status 
//--------------------------------------------------------------------+
StackType_t  g_status_task_stack[STATUS_TASK_STACK_SIZE];
StaticTask_t g_status_task_buf;

static void status_task(__unused void *params) 
{
    bool on = false;
    TickType_t next = xTaskGetTickCount();

    while (true) {
        if (g_usb_device_configured) {
            vTaskDelayUntil(&next, pdMS_TO_TICKS(250));
            status_led_put(on ? STATUS_COL_BLUE : STATUS_COL_BLACK);
            on = !on;
        }
        else {
            vTaskDelayUntil(&next, pdMS_TO_TICKS(100));
            status_led_put(on ? STATUS_COL_RED : STATUS_COL_BLACK);
            on = !on;
        }
    }
}

#if configUSE_TRACE_FACILITY

void print_tasks_info();


static void statistics_task(__unused void *params)
{
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        #if 1
        print_tasks_info();
        #endif
        printf("Uptime: %u seconds from core %u\n", xTaskGetTickCount() / configTICK_RATE_HZ, get_core_num());
    }
}
#endif


//--------------------------------------------------------------------+
// USB Device
//--------------------------------------------------------------------+

static TaskHandle_t g_usb_task = NULL;
static StaticTask_t g_usb_task_buf;
static StackType_t  g_usb_task_stack[USBD_TASK_STACK_SIZE];

static void usb_device_task(__unused void *param)
{
    printf("USB task started on core %u\n", get_core_num());

    vTaskDelay(pdMS_TO_TICKS(100));

    tud_init(TUD_OPT_RHPORT);

    pio_uart_init();
    keyboard_init();

    while (true) {
        tud_task();
        watchdog_update();
    }
}

// Invoked when device is mounted (configured)
void tud_mount_cb(void)
{
    g_usb_device_configured = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    g_usb_device_configured = false;
}



//--------------------------------------------------------------------+
// FreeRTOS Hooks
//--------------------------------------------------------------------+

#if configUSE_PASSIVE_IDLE_HOOK

volatile uint32_t g_passive_idle_counter = 0;

void vApplicationIdleHook()
{
    static uint32_t last_count = 0;

    // Only update watchdog if the other core is incrementing the idle counter
    if (last_count!=g_passive_idle_counter) {
        watchdog_update();
        last_count = g_passive_idle_counter;
    }
}

void vApplicationPassiveIdleHook()
{
    g_passive_idle_counter++;
}

#else

void vApplicationIdleHook()
{
    watchdog_update();
}

#endif


//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+


static void app_main(__unused void *params)
{
    printf("Starting main\n");

    // Clear all neopixels
    neopixel_strip_set_enabled(NEOPIXEL_STRIP1, true);
    neopixel_strip_fill(NEOPIXEL_STRIP1, NEOPIXEL_BLACK);
    neopixel_strip_show(NEOPIXEL_STRIP1);


    #ifdef USBD_TASK_CORE_AFFINITY
    g_usb_task = xTaskCreateStaticAffinitySet(usb_device_task, USBD_TASK_NAME, USBD_TASK_STACK_SIZE, NULL, USBD_TASK_PRIORITY, g_usb_task_stack, &g_usb_task_buf, USBD_TASK_CORE_AFFINITY);
    #else
    g_usb_task = xTaskCreateStatic(usb_device_task, USBD_TASK_NAME, USBD_TASK_STACK_SIZE, NULL, USBD_TASK_PRIORITY, g_usb_task_stack, &g_usb_task_buf);
    #endif
    xTaskCreateStatic(status_task, STATUS_TASK_NAME, STATUS_TASK_STACK_SIZE, NULL, STATUS_TASK_PRIORITY, g_status_task_stack, &g_status_task_buf);

    //xTaskCreate(test_task, "test", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, NULL);


    #if configUSE_TRACE_FACILITY
    xTaskCreate(statistics_task, "statistics", configMINIMAL_STACK_SIZE, NULL, STATISTICS_TASK_PRIORITY, NULL);
    #endif

    printf("Main done\n");
    vTaskDelete(NULL);
}


int main(void) {
    board_init();

    printf("-----------------------------------\n");
    #if ( configNUMBER_OF_CORES > 1 )
        printf("Starting FreeRTOS SMP on %d cores\n", configNUMBER_OF_CORES);
    #else
        printf("Starting FreeRTOS on core %u\n", get_core_num());
    #endif
    printf("-----------------------------------\n");

    #if configUSE_CORE_AFFINITY
    xTaskCreateAffinitySet(app_main, "app_main", configMINIMAL_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, (1UL<<0), NULL);
    #else
    xTaskCreate(app_main, "app_main", configMINIMAL_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL);
    #endif

    /* Start the tasks and timer running. */
    printf("Starting task scheduler\n");
    vTaskStartScheduler();

    printf("Error: Failed to start scheduler\n");
    watchdog_reboot(0, 0, 1000);

    return 0;
}