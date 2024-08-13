#include "pio_uart.h"

#include <stdio.h>
#include <FreeRTOS.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <hardware/pio.h>
#include <tusb.h>
#include <task.h>

#include "board_config.h"
#include "cdc_device.h"
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"


//--------------------------------------------------------------------+
// Config
//--------------------------------------------------------------------+

#define UART_DEFAULT_BAUD 9600
#define UART_FIFO_SIZE 64

#define UART_TASK_NAME "pio_uart"
#define UART_TASK_RX_EVENT (1<<0)
#define UART_TASK_TX_EVENT (1<<1)

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
    queue_t rx_fifo;

    uint    tx_sm;
    queue_t tx_fifo;
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

static StackType_t  g_task_stack[UART_PIO_TASK_STACK_SIZE];
static StaticTask_t g_task_def;
static TaskHandle_t g_task;

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


static void __isr pio_rx_irq_func()
{
    uint rx_cnt = 0;

    // Loop through devices and move PIO RX data to fifo
    for (uint i=0; i<N_DEVICES; i++) {
        struct device_data_t *data = &g_device_data[i];

        // Check RX FIFO
        while (!pio_sm_is_rx_fifo_empty(UART_PIO, data->rx_sm)) {
            char c = uart_rx_program_getc(UART_PIO, data->rx_sm);
            if (queue_try_add(&data->rx_fifo, &c)) {
                rx_cnt++;
            }
            else {
                DEBUG_PANIC("RX fifo full");
            }
        }
    }
    if (rx_cnt) {
        //printf("RX IRQ: %u\n", rx_cnt);
        xTaskNotifyFromISR(g_task, UART_TASK_RX_EVENT, eSetBits, NULL);
    }
}


static void __isr pio_tx_irq_func()
{
    // Loop through devices and fill PIO TX data from fifo
    for (uint i=0; i<N_DEVICES; i++) {
        struct device_data_t *data = &g_device_data[i];
        uint tx_cnt = 0;

        // Check TX FIFO
        while (!queue_is_empty(&data->tx_fifo)) {
            if (pio_sm_is_tx_fifo_full(UART_PIO, data->tx_sm)) {
                break;
            }
            uint8_t c;
            if (queue_try_remove(&data->tx_fifo, &c)) {
                uart_tx_program_putc(UART_PIO, data->tx_sm, c);
                tx_cnt++;
            }
        }
        if (queue_is_empty(&data->tx_fifo)) {
            // TX Fifo is empty, disable interrupt and trigger TX worker to queue more data if applicable
            pio_txnfull_set_enabled(data, false);
            if (tx_cnt) {
                xTaskNotifyFromISR(g_task, UART_TASK_TX_EVENT, eSetBits, NULL);
            }
        }
    }
}


/**
 * Move data from PIO UART to CDC 
 */
static inline void worker_do_rx()
{
    printf("RX Worker>> %u\n", get_core_num());
    for (uint i=0; i<N_DEVICES; i++) {
        const struct device_t *dev = &g_devices[i];
        struct device_data_t *data = &g_device_data[i];
        uint wr = 0;

        while (!queue_is_empty(&data->rx_fifo)) {
            if (tud_cdc_n_write_available(dev->cdc_itf)) {
                uint8_t c;
                if (!queue_try_remove(&data->rx_fifo, &c)) {
                    panic("RX fifo empty");
                }
                tud_cdc_n_write_char(dev->cdc_itf, c);
                wr++;
            }
            else {
                // Output buffer full
                break;
            }
        }

        if (wr) {
            //printf("< wrote=%u\n", wrote);
            tud_cdc_n_write_flush(dev->cdc_itf);
        }
    }
    //printf("RX Worker<<\n");
}


/**
 * Move data from CDC to PIO UART
 */
static inline void worker_do_tx()
{
    //printf("%u TX Worker>>\n", get_core_num());
    uint8_t ch;
    uint32_t rd;

    for (uint i=0; i<N_DEVICES; i++) {
        const struct device_t *dev = &g_devices[i];
        struct device_data_t *data = &g_device_data[i];

        while (tud_cdc_n_available(dev->cdc_itf)) {
            if (queue_is_full(&data->tx_fifo)) {
                break;
            }
            rd = tud_cdc_n_read(dev->cdc_itf, &ch, 1);
            if (rd) {
                if (!queue_try_add(&data->tx_fifo, &ch)) {
                    DEBUG_PANIC("TX fifo full");
                }
            }
        }

        // If the TX fifo is not empty enable interrupt that moves data to PIO 
        if (!queue_is_empty(&data->tx_fifo)) {
            pio_txnfull_set_enabled(data, true);
        }
    }
    //printf("%u TX Worker<<\n", get_core_num());
}


static void worker_task_func(__unused void *params)
{
    uint32_t events = 0;

    // Initialize interrupts and restore affinity
    pio_uart_irq_init();
    #ifdef UART_PIO_TASK_CORE_AFFINITY
    vTaskCoreAffinitySet(NULL, UART_PIO_TASK_CORE_AFFINITY);
    #elif configUSE_CORE_AFFINITY
    vTaskCoreAffinitySet(NULL, configTASK_DEFAULT_CORE_AFFINITY);
    #endif


    printf("PIO UART Worker running\n");

    while (true) {
        events = 0;
        xTaskNotifyWait(pdFALSE, UINT32_MAX, &events, portMAX_DELAY);

        if (events & UART_TASK_RX_EVENT) {
            worker_do_rx();
        }
        if (events & UART_TASK_TX_EVENT) {
            worker_do_tx();
        }
    }
}





//--------------------------------------------------------------------+
// CDC Callbacks
//--------------------------------------------------------------------+

static void pio_uart_cdc_rx_cb(uint8_t itf, void *data)
{
    //printf("%d CDC RX %u\n", get_core_num(), itf);
    xTaskNotify(g_task, UART_TASK_TX_EVENT, eSetBits);
}


static void pio_uart_cdc_tx_complete_cb(uint8_t itf, void *data)
{
    //printf("%u CDC TX Complete %u\n", get_core_num(), itf);
    xTaskNotify(g_task, UART_TASK_RX_EVENT, eSetBits);
}


static void pio_uart_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *line_coding, void *_data)
{
    struct device_data_t *data = _data;
    //printf("CDC Line coding: baud=%u  data_bits=%u  parity=%u  stop_bits=%u\n", line_coding->bit_rate, line_coding->data_bits, line_coding->parity, line_coding->stop_bits);

    if (data->baud != line_coding->bit_rate) {
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
        queue_init(&data->rx_fifo, 1, UART_FIFO_SIZE);
        queue_init(&data->tx_fifo, 1, UART_FIFO_SIZE);

        data->rx_sm = pio_claim_unused_sm(UART_PIO, true);
        uart_rx_program_init(UART_PIO, data->rx_sm, g_rx_program_offset, dev->rx_pin, data->baud);
        
        data->tx_sm = pio_claim_unused_sm(UART_PIO, true);
        uart_tx_program_init(UART_PIO, data->tx_sm, g_tx_program_offset, dev->tx_pin, data->baud);

        usb_cdc_device_set_rx_callback(dev->cdc_itf, pio_uart_cdc_rx_cb, data);
        usb_cdc_device_set_tx_complete_callback(dev->cdc_itf, pio_uart_cdc_tx_complete_cb, data);
        usb_cdc_device_set_line_coding_callback(dev->cdc_itf, pio_uart_cdc_line_coding_cb, data);

        pio_set_irqn_source_enabled(UART_PIO, UART_PIO_RX_IRQ_IDX, pis_sm0_rx_fifo_not_empty + data->rx_sm, true);
    }

    g_task = xTaskCreateStatic(worker_task_func, UART_TASK_NAME, UART_PIO_TASK_STACK_SIZE, NULL, UART_PIO_TASK_PRIORITY, g_task_stack, &g_task_def);
    #ifdef UART_PIO_IRQ_CORE
    vTaskCoreAffinitySet(g_task, (1UL<<UART_PIO_IRQ_CORE));
    #endif
}

