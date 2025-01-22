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
#define UART_FIFO_SIZE 512

#define UART_TASK_NAME "pio_uart_%u"
#define UART_TASK_RX_EVENT (1<<0)
#define UART_TASK_TX_EVENT (1<<1)
#define UART_TASK_TIMEOUT (pdMS_TO_TICKS(100))
#define UART_TASK_FLUSH_TIMEOUT (pdMS_TO_TICKS(1))

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

    TaskHandle_t task;
    StackType_t  task_stack[UART_PIO_TASK_STACK_SIZE];
    StaticTask_t task_def;
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
    // Loop through devices and move PIO RX data to fifo
    for (uint i=0; i<N_DEVICES; i++) {
        struct device_data_t *data = &g_device_data[i];
        uint rx_cnt = 0;

        // Check RX FIFO
        while (!pio_sm_is_rx_fifo_empty(UART_PIO, data->rx_sm)) {
            char c = uart_rx_program_getc(UART_PIO, data->rx_sm);
            if (queue_try_add(&data->rx_fifo, &c)) {
                rx_cnt++;
            }
            else {
                //DEBUG_PANIC("RX fifo full");
                rx_cnt++;
            }
        }

        if (rx_cnt) {
            if (rx_cnt > rx_high) {
                rx_high = rx_cnt;
            }
            //printf("RX IRQ: %u\n", rx_cnt);
            xTaskNotifyFromISR(data->task, UART_TASK_RX_EVENT, eSetBits, NULL);
        }
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
                xTaskNotifyFromISR(data->task, UART_TASK_TX_EVENT, eSetBits, NULL);
            }
        }
    }
}


/**
 * Move data from PIO UART to CDC 
 */
static inline void worker_do_rx(struct device_data_t *data)
{
    const struct device_t *dev = data->dev;
    uint wr = 0;

    //printf("RX Worker %u>> %u\n", dev->cdc_itf, get_core_num());

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
}


static inline void worker_do_rx_flush(struct device_data_t *data)
{
    const struct device_t *dev = data->dev;
    uint drop = 0;
    uint8_t c;
 
    while (!queue_is_empty(&data->rx_fifo)) {
        queue_try_remove(&data->rx_fifo, &c);
        drop++;
    }
    if (drop) {
        printf("CDC not ready, discarding %u\n", drop);
    }
}


/**
 * Move data from CDC to PIO UART
 */
static inline void worker_do_tx(struct device_data_t *data)
{
    const struct device_t *dev = data->dev;
    uint8_t ch;
    uint32_t rd;
    uint32_t wr = 0;

    //printf("TX Worker %u>> %u\n", dev->cdc_itf, get_core_num());

    while (tud_cdc_n_available(dev->cdc_itf)) {
        if (queue_is_full(&data->tx_fifo)) {
            break;
        }
        rd = tud_cdc_n_read(dev->cdc_itf, &ch, 1);
        if (rd) {
            if (!queue_try_add(&data->tx_fifo, &ch)) {
                DEBUG_PANIC("TX fifo full");
            }
            wr++;
        }
    }

    // If the TX fifo is not empty enable interrupt that moves data to PIO 
    if (!queue_is_empty(&data->tx_fifo)) {
        pio_txnfull_set_enabled(data, true);
    }

    //if (wr) {
    //    printf(">%u wrote=%u\n", dev->cdc_itf, wr);
    //}    

    //printf("TX Worker %u<<\n", dev->cdc_itf);
}


static void worker_task_func(__unused void *params)
{
    struct device_data_t *data = params;
    const struct device_t *dev = data->dev;
    TickType_t timeout = UART_TASK_TIMEOUT;
    TickType_t last_flush = xTaskGetTickCount();
    TickType_t now;
    uint32_t events = 0;
    bool flush_pending = false;

    printf("PIO UART Worker %u running on core %d\n", data->dev->cdc_itf, get_core_num());

    while (true) {
        events = 0;

        xTaskNotifyWait(0U, UINT32_MAX, &events, timeout);
        timeout = UART_TASK_TIMEOUT;
        now = xTaskGetTickCount();

        if (events & UART_TASK_RX_EVENT) {
            if (tud_cdc_n_ready(dev->cdc_itf)) {
                worker_do_rx(data);
                if (tud_cdc_n_available(dev->cdc_itf) < CFG_TUD_CDC_RX_BUFSIZE) {
                    last_flush = now;
                    timeout = UART_TASK_FLUSH_TIMEOUT;
                    flush_pending = true;
                }
                else {
                    flush_pending = false;
                }
            }
            else {
                flush_pending = false;
                worker_do_rx_flush(data);
            }
        }

        if (events & UART_TASK_TX_EVENT) {
            worker_do_tx(data);
        }

        if (flush_pending && (now - last_flush > UART_TASK_FLUSH_TIMEOUT)) {
            last_flush = now;
            flush_pending = false;
            tud_cdc_n_write_flush(dev->cdc_itf);
        }
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
    //printf("%d CDC RX %u\n", get_core_num(), itf);
    xTaskNotify(dev->task, UART_TASK_TX_EVENT, eSetBits);
}


static void pio_uart_cdc_tx_complete_cb(uint8_t itf, void *data)
{
    struct device_data_t *dev = data;
    //printf("%u CDC TX Complete %u\n", get_core_num(), itf);
    xTaskNotify(dev->task, UART_TASK_RX_EVENT, eSetBits);
}


static void pio_uart_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *line_coding, void *_data)
{
    struct device_data_t *data = _data;
    printf("CDC Line coding: baud=%u  data_bits=%u  parity=%u  stop_bits=%u\n", line_coding->bit_rate, line_coding->data_bits, line_coding->parity, line_coding->stop_bits);

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

        char task_name[16];
        sprintf(task_name, UART_TASK_NAME, i);

        #ifdef UART_PIO_TASK_CORE_AFFINITY
        data->task = xTaskCreateStaticAffinitySet(worker_task_func, task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->task_stack, &data->task_def, UART_PIO_IRQ_CORE_AFFINITY);
        #else
        data->task = xTaskCreateStatic(worker_task_func, task_name, UART_PIO_TASK_STACK_SIZE, data, UART_PIO_TASK_PRIORITY, data->task_stack, &data->task_def);
        #endif
    }

    #ifdef UART_PIO_IRQ_CORE
    if (get_core_num() != UART_PIO_IRQ_CORE) {
        xTaskCreateAffinitySet(irq_task_func, "pio_irq", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL, UART_PIO_IRQ_CORE_AFFINITY);
    }
    else {
        pio_uart_irq_init();
    }
    #else
    pio_uart_irq_init();
    #endif
}

