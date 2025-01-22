#pragma once

#include <FreeRTOS.h>
#include <tusb.h>
#include "pins.h"


// ----------------------------------------------------------------------
// Task priorities
// ----------------------------------------------------------------------
#define USBD_TASK_PRIORITY                 ( configMAX_PRIORITIES - 2UL )
#define UART_PIO_TASK_PRIORITY             ( configMAX_PRIORITIES - 3UL )
#define STATUS_TASK_PRIORITY               ( tskIDLE_PRIORITY + 1 )


// ----------------------------------------------------------------------
// USB Device
// ----------------------------------------------------------------------
#define USBD_TASK_STACK_SIZE               ( configMINIMAL_STACK_SIZE )
#if configUSE_CORE_AFFINITY
//#define USBD_TASK_CORE_AFFINITY            configTASK_DEFAULT_CORE_AFFINITY
#define USBD_TASK_CORE 0
#define USBD_TASK_CORE_AFFINITY            (1UL<<USBD_TASK_CORE)
#endif


// ----------------------------------------------------------------------
// Neopixel
// ----------------------------------------------------------------------
#define NEOPIXEL_PIO        pio0
#define NEOPIXEL_DMA_IRQ    DMA_IRQ_0
#define NEOPIXEL_STRIP_COUNT 2

#define NEOPIXEL_STRIP0_PIN         XIAO_NEOPIXEL_PIN
#define NEOPIXEL_STRIP0_POWER_PIN   XIAO_NEOPIXEL_POWER_PIN
#define NEOPIXEL_STRIP0_COUNT       1
#define NEOPIXEL_STRIP0_FREQUENCY   800000u
#define NEOPIXEL_STRIP0_RESET_US    60u // Spec says min 50us, so we use 60 just to be sure
#define NEOPIXEL_STRIP0_RGBW        false
#define NEOPIXEL_STRIP0_USE_DMA     false

#define NEOPIXEL_STRIP1_PIN         XIAO_D0_PIN
#define NEOPIXEL_STRIP1_POWER_PIN   INVALID_PIN
#define NEOPIXEL_STRIP1_COUNT       16
#define NEOPIXEL_STRIP1_FREQUENCY   800000
#define NEOPIXEL_STRIP1_RESET_US    60u
#define NEOPIXEL_STRIP1_RGBW        false
#define NEOPIXEL_STRIP1_USE_DMA     true



// ----------------------------------------------------------------------
// HID
// ----------------------------------------------------------------------
/* Buttons */
#define BUTTON_SW1_PIN XIAO_D4
#define BUTTON_SW2_PIN XIAO_D5

#define HID_NEOPIXEL_STRIP 1
#define HID_NEOPIXEL_COUNT NEOPIXEL_STRIP1_COUNT



// ----------------------------------------------------------------------
// UART
// ----------------------------------------------------------------------
#define UART_PIO            pio1
#define UART_PIO_IRQ_BASE   PIO1_IRQ_0
#define UART_PIO_TASK_STACK_SIZE           256
#if configUSE_CORE_AFFINITY
#define UART_PIO_TASK_CORE_AFFINITY        USBD_TASK_CORE_AFFINITY
#define UART_PIO_IRQ_CORE                  0
#define UART_PIO_IRQ_CORE_AFFINITY         (1UL<<UART_PIO_IRQ_CORE)
#endif

#if CFG_TUD_CDC > 0
#define PIO_UART0_CDC 0
#define PIO_UART0_RX_PIN XIAO_D10_PIN
#define PIO_UART0_TX_PIN XIAO_D9_PIN
#endif

#if CFG_TUD_CDC > 1
#define PIO_UART1_CDC 1
#define PIO_UART1_RX_PIN XIAO_D3_PIN
#define PIO_UART1_TX_PIN XIAO_D2_PIN
#endif


// ----------------------------------------------------------------------
// Status LED
// ----------------------------------------------------------------------
#define STATUS_RGB_LED 0
#define STATUS_NEOPIXEL_LED 0

#ifdef SEEED_XIAO_RP2040
#define STATUS_LED_R_PIN          XIAO_LED_R_PIN
#define STATUS_LED_G_PIN          XIAO_LED_G_PIN
#define STATUS_LED_B_PIN          XIAO_LED_B_PIN
#define STATUS_LED_ACTIVE_LOW     PICO_DEFAULT_LED_PIN_INVERTED
#endif


// ----------------------------------------------------------------------
// Watchdog
// ----------------------------------------------------------------------
#define WATCHDOG_INTERVAL_MS    100

// ----------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------

#if NDEBUG
#define DEBUG_PANIC(X...) 
#else
#define DEBUG_PANIC(X...) panic(X)
#endif
