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

#include "pio_uart.h"

#include <stdio.h>
#include <FreeRTOS.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <hardware/pio.h>
#include <task.h>
#include <stream_buffer.h>
#include <tusb.h>

#include "board_config.h"
#include "cdc_device.h"
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"


//--------------------------------------------------------------------+
// Config
//--------------------------------------------------------------------+

#define UART_DEFAULT_BAUD 9600
#define UART_FIFO_SIZE 512

#define UART_TASK_NAME "pio_uart%u_%s"
#define UART_TASK_FLUSH_INTERVAL (pdMS_TO_TICKS(4))

#define UART_PIO_RX_IRQ     (UART_PIO_IRQ_BASE)
#define UART_PIO_RX_IRQ_IDX (UART_PIO_RX_IRQ-UART_PIO_IRQ_BASE)

#define UART_PIO_TX_IRQ     (UART_PIO_IRQ_BASE+1)
#define UART_PIO_TX_IRQ_IDX (UART_PIO_TX_IRQ-UART_PIO_IRQ_BASE)


// Validate that PIO interrupts are contiguous
static_assert(PIO0_IRQ_0 + 1 == PIO0_IRQ_1);
static_assert(PIO1_IRQ_0 + 1 == PIO1_IRQ_1);
static_assert(UART_PIO_TASK_PRIORITY > tskIDLE_PRIORITY);

// Validate that PIO interrupt sources are contiguous
static_assert(pis_sm0_rx_fifo_not_empty + 1 == pis_sm1_rx_fifo_not_empty);
static_assert(pis_sm0_rx_fifo_not_empty + 2 == pis_sm2_rx_fifo_not_empty);
static_assert(pis_sm0_rx_fifo_not_empty + 3 == pis_sm3_rx_fifo_not_empty);
static_assert(pis_sm0_tx_fifo_not_full + 1 == pis_sm1_tx_fifo_not_full);
static_assert(pis_sm0_tx_fifo_not_full + 2 == pis_sm2_tx_fifo_not_full);
static_assert(pis_sm0_tx_fifo_not_full + 3 == pis_sm3_tx_fifo_not_full);



//--------------------------------------------------------------------+
// Types
//--------------------------------------------------------------------+

struct device_t {
    uint cdc_itf;

    uint rx_pin;
    uint tx_pin;
};

struct device_data_t {
    const struct device_t *dev;

    uint baud;

    uint    rx_sm;
    uint    tx_sm;

    StreamBufferHandle_t rx_fifo;
    StaticStreamBuffer_t rx_fifo_def;
    uint8_t              rx_fifo_storage[UART_FIFO_SIZE];

    TaskHandle_t rx_task;
    StaticTask_t rx_task_def;
    StackType_t  rx_task_stack[UART_PIO_TASK_STACK_SIZE];

    StreamBufferHandle_t tx_fifo;
    StaticStreamBuffer_t tx_fifo_def;
    uint8_t              tx_fifo_storage[UART_FIFO_SIZE];

    TaskHandle_t tx_task;
    StaticTask_t tx_task_def;
    StackType_t  tx_task_stack[UART_PIO_TASK_STACK_SIZE];
};


//--------------------------------------------------------------------+
// Variables
//--------------------------------------------------------------------+

static const struct device_t g_devices[] = {
    #ifdef PIO_UART0_CDC
    {
        .cdc_itf = PIO_UART0_CDC,
        .rx_pin  = PIO_UART0_RX_PIN,
        .tx_pin  = PIO_UART0_TX_PIN,
    },
    #endif
    #ifdef PIO_UART1_CDC
    {
        .cdc_itf = PIO_UART1_CDC,
        .rx_pin  = PIO_UART1_RX_PIN,
        .tx_pin  = PIO_UART1_TX_PIN,
    },
    #endif
};
#define N_DEVICES (count_of(g_devices))

static struct device_data_t g_device_data[N_DEVICES];


static uint g_tx_program_offset;
static uint g_rx_program_offset;


//--------------------------------------------------------------------+
// Interrupts and workers
//--------------------------------------------------------------------+

static void pio_uart_irq_init();


static inline void pio_txnfull_set_enabled(const struct device_data_t *data, bool enabled)
{
    pio_set_irqn_source_enabled(UART_PIO, UART_PIO_TX_IRQ_IDX, pis_sm0_tx_fifo_not_full + data->tx_sm, enabled); 
}


volatile uint rx_high = 0;

static void __isr pio_rx_irq_func()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Loop through devices and move PIO RX data to fifo
    for (uint i=0; i<N_DEVICES; i++) {
        struct device_data_t *data = &g_device_data[i];

        // Check RX FIFO
        while (!pio_sm_is_rx_fifo_empty(UART_PIO, data->rx_sm)) {
            char c = uart_rx_program_getc(UART_PIO, data->rx_sm);
            xStreamBufferSendFromISR(data->rx_fifo, &c, 1, &xHigherPriorityTaskWoken);
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


static void __isr pio_tx_irq_func()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Loop through devices and fill PIO TX data from fifo
    for (uint i=0; i<N_DEVICES; i++) {
        struct device_data_t *data = &g_device_data[i];

        // Check TX FIFO
        while (!xStreamBufferIsEmpty(data->tx_fifo)) {
            if (pio_sm_is_tx_fifo_full(UART_PIO, data->tx_sm)) {
                break;
            }
            uint8_t c;
            if (xStreamBufferReceiveFromISR(data->tx_fifo, &c, 1, &xHigherPriorityTaskWoken)) {
                uart_tx_program_putc(UART_PIO, data->tx_sm, c);
            }
        }
        if (xStreamBufferIsEmpty(data->tx_fifo)) {
            // TX Fifo is empty, disable interrupt and trigger TX worker to queue more data if applicable
            pio_txnfull_set_enabled(data, false);
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**
 * Move data from PIO UART to CDC 
 */
static void worker_rx_task_func(__unused void *params)
{
    struct device_data_t *data = params;
    const struct device_t *dev = data->dev;
    uint8_t buffer[CFG_TUD_CDC_TX_BUFSIZE];
    uint8_t *buffer_ptr = buffer;
    size_t buffer_sz;

    printf("PIO UART%u RX Worker %u running\n", data->dev->cdc_itf);

    while (true) {
        buffer_sz = xStreamBufferReceive(data->rx_fifo, buffer, sizeof(buffer), UART_TASK_FLUSH_INTERVAL);

        if (buffer_sz>0 && tud_cdc_n_ready(dev->cdc_itf)) {
            //printf("RX Worker %u>> rc=%u\n", dev->cdc_itf, rc);

            buffer_ptr = buffer;
            while (buffer_sz) {
                size_t wr = tud_cdc_n_write(dev->cdc_itf, buffer_ptr, buffer_sz);
                buffer_sz -= wr;
                buffer_ptr += wr;
                if (buffer_sz && !tud_cdc_n_write_available(dev->cdc_itf)) {
                    tud_cdc_n_write_flush(dev->cdc_itf);
                    xTaskNotifyWait(0U, UINT32_MAX, NULL, UART_TASK_FLUSH_INTERVAL);
                }
            }

            tud_cdc_n_write_flush(dev->cdc_itf);

            //printf("RX Worker %u<<\n", dev->cdc_itf);
        }
    }
}


/**
 * Move data from CDC to PIO UART
 */
static void worker_tx_task_func(__unused void *params)
{
    struct device_data_t *data = params;
    const struct device_t *dev = data->dev;
    uint8_t buffer[CFG_TUD_CDC_RX_BUFSIZE];
    uint8_t *buffer_ptr = buffer;
    size_t buffer_sz;

    printf("PIO UART%u TX Worker running\n", data->dev->cdc_itf);

    while (true) {
        xTaskNotifyWait(0U, UINT32_MAX, NULL, portMAX_DELAY);

        //printf("TX Worker %u>>\n", dev->cdc_itf);

        while (tud_cdc_n_available(dev->cdc_itf)) {
            buffer_sz = tud_cdc_n_read(dev->cdc_itf, buffer, sizeof(buffer));
            //printf(">%u read=%u\n", dev->cdc_itf, buffer_sz);

            buffer_ptr = buffer;
            while (buffer_sz) {
                if (!xStreamBufferIsEmpty(data->tx_fifo)) {
                    pio_txnfull_set_enabled(data, true);
                }
                size_t wr = xStreamBufferSend(data->tx_fifo, buffer_ptr, buffer_sz, portMAX_DELAY);
                buffer_sz -= wr;
                buffer_ptr += wr;
            }
        }

        // If the TX fifo is not empty enable interrupt that moves data to PIO 
        if (!xStreamBufferIsEmpty(data->tx_fifo)) {
            pio_txnfull_set_enabled(data, true);
        }

        //printf("TX Worker %u<<\n", dev->cdc_itf);
    }
}



static void irq_task_func(__unused void *params)
{
    pio_uart_irq_init();
    vTaskDelete(NULL);
}


//--------------------------------------------------------------------+
// CDC Callbacks
//--------------------------------------------------------------------+

static void pio_uart_cdc_rx_cb(uint8_t itf, void *data)
{
    struct device_data_t *dev = data;
    //printf("%s[%d] CDC RX %u\n", pcTaskGetName(NULL), get_core_num(), itf);
    xTaskNotify(dev->tx_task, 1, eSetBits);
}


static void pio_uart_cdc_tx_complete_cb(uint8_t itf, void *data)
{
    struct device_data_t *dev = data;
    //printf("%s[%u] CDC TX Complete %u\n", pcTaskGetName(NULL), get_core_num(), itf);
    xTaskNotify(dev->rx_task, 1, eSetBits);
}


void pio_uart_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts, void *_data)
{
    struct device_data_t *data = _data;
    printf("%s[%u] CDC State: DTR=%u RTS=%u\n", pcTaskGetName(NULL), get_core_num(), dtr, rts);
}


static void pio_uart_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *line_coding, void *_data)
{
    struct device_data_t *data = _data;
    printf("%s[%u] CDC Line coding: baud=%u  data_bits=%u  parity=%u  stop_bits=%u\n", pcTaskGetName(NULL), get_core_num(), line_coding->bit_rate, line_coding->data_bits, line_coding->parity, line_coding->stop_bits);

    if (line_coding->bit_rate && data->baud != line_coding->bit_rate) {
        printf("CDC Baud changed %u -> %u\n", data->baud, line_coding->bit_rate);
        uart_rx_program_set_baud(UART_PIO, data->rx_sm, line_coding->bit_rate);
        uart_tx_program_set_baud(UART_PIO, data->tx_sm, line_coding->bit_rate);
        data->baud = line_coding->bit_rate;
    }
}





//--------------------------------------------------------------------+
// Init
//--------------------------------------------------------------------+

static void pio_uart_irq_init()
{
    printf("PIO UART IRQ registered on core %u\n", get_core_num());

    irq_set_exclusive_handler(UART_PIO_RX_IRQ, pio_rx_irq_func);
    irq_set_exclusive_handler(UART_PIO_TX_IRQ, pio_tx_irq_func);
    irq_set_priority(UART_PIO_RX_IRQ, PICO_HIGHEST_IRQ_PRIORITY);

    irq_set_enabled(UART_PIO_RX_IRQ, true);
    irq_set_enabled(UART_PIO_TX_IRQ, true);
}

void pio_uart_init()
{
    printf("PIO UART Init\n");

    g_rx_program_offset = pio_add_program(UART_PIO, &uart_rx_program);
    g_tx_program_offset = pio_add_program(UART_PIO, &uart_tx_program);

    for (uint i=0; i<N_DEVICES; i++) {
        const struct device_t *dev = &g_devices[i];
        struct device_data_t *data = &g_device_data[i];

        data->dev = dev;
        data->baud = UART_DEFAULT_BAUD;
        data->rx_fifo = xStreamBufferCreateStatic(sizeof(data->rx_fifo_storage), 1, data->rx_fifo_storage, &data->rx_fifo_def);
        data->tx_fifo = xStreamBufferCreateStatic(sizeof(data->tx_fifo_storage), 1, data->tx_fifo_storage, &data->tx_fifo_def);

        data->rx_sm = pio_claim_unused_sm(UART_PIO, true);
        uart_rx_program_init(UART_PIO, data->rx_sm, g_rx_program_offset, dev->rx_pin, data->baud);
        
        data->tx_sm = pio_claim_unused_sm(UART_PIO, true);
        uart_tx_program_init(UART_PIO, data->tx_sm, g_tx_program_offset, dev->tx_pin, data->baud);

        usb_cdc_device_set_rx_callback(dev->cdc_itf, pio_uart_cdc_rx_cb, data);
        usb_cdc_device_set_tx_complete_callback(dev->cdc_itf, pio_uart_cdc_tx_complete_cb, data);
        usb_cdc_device_set_line_state_callback(dev->cdc_itf, pio_uart_cdc_line_state_cb, data);
        usb_cdc_device_set_line_coding_callback(dev->cdc_itf, pio_uart_cdc_line_coding_cb, data);

        pio_set_irqn_source_enabled(UART_PIO, UART_PIO_RX_IRQ_IDX, pis_sm0_rx_fifo_not_empty + data->rx_sm, true);

        char rx_task_name[16];
        char tx_task_name[16];
        sprintf(rx_task_name, UART_TASK_NAME, i, "rx");
        sprintf(tx_task_name, UART_TASK_NAME, i, "tx");
        #ifdef UART_PIO_TASK_CORE_AFFINITY
        data->rx_task = xTaskCreateStaticAffinitySet(worker_rx_task_func, rx_task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->rx_task_stack, &data->rx_task_def, UART_PIO_TASK_CORE_AFFINITY);
        data->tx_task = xTaskCreateStaticAffinitySet(worker_tx_task_func, tx_task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->tx_task_stack, &data->tx_task_def, UART_PIO_TASK_CORE_AFFINITY);
        #else
        data->rx_task = xTaskCreateStatic(worker_rx_task_func, rx_task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->rx_task_stack, &data->rx_task_def);
        data->tx_task = xTaskCreateStatic(worker_tx_task_func, tx_task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->tx_task_stack, &data->tx_task_def);
        #endif
    }

    #ifdef UART_PIO_IRQ_CORE
    // If we are not currently running on the irq core, create a task to handle to set up the irq
    if (get_core_num() != UART_PIO_IRQ_CORE) {
        xTaskCreateAffinitySet(irq_task_func, "pio_irq", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, UART_PIO_IRQ_CORE_AFFINITY, NULL);
    }
    else {
        pio_uart_irq_init();
    }
    #else
    pio_uart_irq_init();
    #endif
}

