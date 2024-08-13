#include "neopixel.h"

#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/irq.h>

#include "ws2812.pio.h"
#include "board_config.h"

struct strip_t {
    uint pin;
    uint power_pin;
    uint n_pixels;
    float bitrate;
    bool rgbw;
    uint reset_us; // time to wait before sending next update (includes time needed to empty the PIO fifo)
};


struct strip_data_t {
    const struct strip_t *strip;

    bool enabled;

    uint sm;
    color_t *pixels;

    StaticSemaphore_t sem_buf;
    SemaphoreHandle_t sem;

    bool     dma_enabled;
    uint     dma_channel;
    color_t *dma_buffer;
    
    alarm_id_t        reset_alarm;
};


static_assert(DMA_IRQ_0 + 1 == DMA_IRQ_1);

#define NEOPIXEL_DMA_IRQ_IDX        (NEOPIXEL_DMA_IRQ-DMA_IRQ_0)
#define NEOPIXEL_PIO_FIFO_DEPTH     8

#define NEOPIXEL_BPP(rgbw) ((rgbw)?32:24)
#define NEOPIXEL_TRANSMIT_US(freq, bpp, count) ((1000000u*bpp*count)/freq)

// Calculate reset delay 
// ws2812 reset delay + the time it takes to empty the PIO fifo
#define NEOPIXEL_RESET_DELAY(base, freq,rgbw,count) ((base) + NEOPIXEL_TRANSMIT_US(freq, NEOPIXEL_BPP(rgbw), MIN(NEOPIXEL_PIO_FIFO_DEPTH, count)))

//--------------------------------------------------------------------+
// Variables
//--------------------------------------------------------------------+

static const struct strip_t g_strips[] = {
    #if NEOPIXEL_STRIP_COUNT > 0
    {
        .pin            = NEOPIXEL_STRIP0_PIN,
        .power_pin      = NEOPIXEL_STRIP0_POWER_PIN,
        .n_pixels       = NEOPIXEL_STRIP0_COUNT,
        .bitrate        = NEOPIXEL_STRIP0_FREQUENCY,
        .rgbw           = NEOPIXEL_STRIP0_RGBW,
        .reset_us = NEOPIXEL_RESET_DELAY(NEOPIXEL_STRIP0_RESET_US, NEOPIXEL_STRIP0_FREQUENCY, NEOPIXEL_STRIP0_RGBW, NEOPIXEL_STRIP0_COUNT),
    },
    #endif
    #if NEOPIXEL_STRIP_COUNT > 1
    {
        .pin            = NEOPIXEL_STRIP1_PIN,
        .power_pin      = NEOPIXEL_STRIP1_POWER_PIN,
        .n_pixels       = NEOPIXEL_STRIP1_COUNT,
        .bitrate        = NEOPIXEL_STRIP1_FREQUENCY,
        .rgbw           = NEOPIXEL_STRIP1_RGBW,
        .reset_us = NEOPIXEL_RESET_DELAY(NEOPIXEL_STRIP1_RESET_US, NEOPIXEL_STRIP1_FREQUENCY, NEOPIXEL_STRIP1_RGBW, NEOPIXEL_STRIP1_COUNT),
    },
    #endif
};
#define N_STRIPS (count_of(g_strips))

#if NEOPIXEL_STRIP_COUNT > 0
static color_t g_strip0_buffer0[NEOPIXEL_STRIP0_COUNT];
#if NEOPIXEL_STRIP0_USE_DMA
static color_t g_strip0_buffer1[NEOPIXEL_STRIP0_COUNT];
#endif
#endif

#if NEOPIXEL_STRIP_COUNT > 1
static color_t g_strip1_buffer0[NEOPIXEL_STRIP1_COUNT];
#if NEOPIXEL_STRIP1_USE_DMA
static color_t g_strip1_buffer1[NEOPIXEL_STRIP1_COUNT];
#endif
#endif

struct strip_data_t g_strip_data[N_STRIPS] = {
    #if NEOPIXEL_STRIP_COUNT > 0
    {
        .pixels = g_strip0_buffer0,
        #if NEOPIXEL_STRIP0_USE_DMA
        .dma_enabled = true,
        .dma_buffer = g_strip0_buffer1,
        #endif
    },
    #endif
    #if NEOPIXEL_STRIP_COUNT > 1
    {
        .pixels = g_strip1_buffer0,
        #if NEOPIXEL_STRIP1_USE_DMA
        .dma_enabled = true,
        .dma_buffer = g_strip1_buffer1,
        #endif
    },
    #endif
};

static uint g_strip_next = 0;
static uint g_program_offset;


static int64_t reset_alarm_func(alarm_id_t id, void *user_data)
{
    struct strip_data_t *data = user_data;

    data->reset_alarm = 0;
    xSemaphoreGiveFromISR(data->sem, NULL);

    return 0;
}


static void schedule_reset_alarm(struct strip_data_t *data)
{
    if (data->reset_alarm) {
        DEBUG_PANIC("Alarm already registered");
        cancel_alarm(data->reset_alarm);
    }
    data->reset_alarm = add_alarm_in_us(data->strip->reset_us, reset_alarm_func, data, true);
    if (data->reset_alarm < 0) {
        // Add alarm failed - release semaphore to avoid deadlock
        DEBUG_PANIC("Failed to get alarm");
        data->reset_alarm = 0;
        xSemaphoreGiveFromISR(data->sem, NULL);
    }
}


static void __isr dma_complete_irq_func()
{
    for (uint i=0; i<N_STRIPS; i++) {
        struct strip_data_t *data = &g_strip_data[i];
        if (data->dma_enabled && dma_irqn_get_channel_status(NEOPIXEL_DMA_IRQ_IDX, data->dma_channel)) {
            const struct strip_t *strip = &g_strips[i];
            dma_irqn_acknowledge_channel(NEOPIXEL_DMA_IRQ_IDX, data->dma_channel);
            schedule_reset_alarm(data);
        }
    }
}


static inline void dma_copy_pixels(struct strip_data_t *data)
{
    const struct strip_t *strip = data->strip;

    // Copy to DMA buffer
    memcpy(data->dma_buffer, data->pixels, strip->n_pixels*sizeof(color_t));

    // Start DMA transfer
    dma_channel_set_read_addr(data->dma_channel, data->dma_buffer, true);
}

static inline void pio_copy_pixels(struct strip_data_t *data)
{
    const struct strip_t *strip = data->strip;

    for (uint i=0; i<strip->n_pixels; i++) {
        pio_sm_put_blocking(NEOPIXEL_PIO, data->sm, data->pixels[i]);
    }
    schedule_reset_alarm(data);
}



//--------------------------------------------------------------------+
// Interface
//--------------------------------------------------------------------+

void neopixel_strip_set_enabled(uint id, bool enabled)
{
    const struct strip_t *strip = &g_strips[id];
    struct strip_data_t *data = &g_strip_data[id];

    if (enabled == data->enabled) {
        return;
    }

    if (strip->power_pin != INVALID_PIN) {
        gpio_put(strip->power_pin, enabled);
    }
    else {
        // No power pin, just set pixels to black when disabling
        if (!enabled) {
            neopixel_strip_fill(id, NEOPIXEL_BLACK);
            neopixel_strip_show(id);
        }
    }

    data->enabled = enabled;
}


bool neopixel_strip_get_enabled(uint id)
{
    struct strip_data_t *data = &g_strip_data[id];
    return data->enabled;
}



void neopixel_strip_fill(uint id, color_t color)
{
    struct strip_data_t *data = &g_strip_data[id];
    for (uint i=0; i<data->strip->n_pixels; i++) {
        data->pixels[i] = color;
    }
}


void neopixel_strip_set(uint id, uint idx, color_t color)
{
    struct strip_data_t *data = &g_strip_data[id];
    data->pixels[idx] = color;
}


void neopixel_strip_show(uint id)
{
    struct strip_data_t *data = &g_strip_data[id];

    if (!data->enabled) {
        return;
    }

    xSemaphoreTake(data->sem, portMAX_DELAY);

    if (data->dma_enabled) {
        dma_copy_pixels(data);
    }
    else {
        pio_copy_pixels(data);
    }

}



void neopixel_init()
{
    g_program_offset = pio_add_program(NEOPIXEL_PIO, &ws2812_program);

    for (uint i=0; i<N_STRIPS; i++) {
        const struct strip_t *strip = &g_strips[i];
        struct strip_data_t *data = &g_strip_data[i];

        assert(data->pixels);

        data->strip = strip;
        data->enabled = false;
        data->sem = xSemaphoreCreateBinaryStatic(&data->sem_buf);
        xSemaphoreGive(data->sem);

        // Make sure output pin is LOW
        gpio_init(strip->pin);
        gpio_set_dir(strip->pin, GPIO_OUT);
        gpio_put(strip->pin, false);

        if (strip->power_pin != INVALID_PIN) {
            gpio_init(strip->power_pin);
            gpio_set_dir(strip->power_pin, GPIO_OUT);
            gpio_put(strip->power_pin, false);
        }

        // Setup PIO 
        data->sm = pio_claim_unused_sm(NEOPIXEL_PIO, true);
        ws2812_program_init(NEOPIXEL_PIO, data->sm, g_program_offset, strip->pin, strip->bitrate, strip->rgbw);

        if (data->dma_enabled) {
            assert(data->dma_buffer);
            data->dma_channel = dma_claim_unused_channel(true);

            dma_channel_config config = dma_channel_get_default_config(data->dma_channel);
            channel_config_set_dreq(&config, pio_get_dreq(NEOPIXEL_PIO, data->sm, true)); 
            channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
            channel_config_set_read_increment(&config, true); // We want DMA reading to PIO
            channel_config_set_write_increment(&config, false); // We don't want DMA writing from PIO

            dma_channel_configure(data->dma_channel,
                                    &config,
                                    &NEOPIXEL_PIO->txf[data->sm],
                                    data->pixels,
                                    strip->n_pixels,
                                    false);
            dma_irqn_set_channel_enabled(NEOPIXEL_DMA_IRQ_IDX, data->dma_channel, true);
        }
    }

    irq_add_shared_handler(NEOPIXEL_DMA_IRQ, dma_complete_irq_func, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(NEOPIXEL_DMA_IRQ, true);
}


