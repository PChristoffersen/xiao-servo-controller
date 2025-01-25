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

#include "cdc_device.h"
#include <tusb.h>
#include <pico/unique_id.h>
#include <FreeRTOS.h>
#include <task.h>
#include "board_config.h"
#include "usb_descriptors.h"

static struct {
    usb_cdc_rx_cb          rx_cb;
    void                  *rx_cb_data;
    usb_cdc_tx_complete_cb tx_complete_cb;
    void                  *tx_complete_cb_data;
    usb_cdc_line_state_cb  line_state_cb;
    void                  *line_state_cb_data;
    usb_cdc_line_coding_cb line_coding_cb;
    void                  *line_coding_cb_data;
} g_cdc_device[CFG_TUD_CDC];




//--------------------------------------------------------------------+
// Interface
//--------------------------------------------------------------------+


void usb_cdc_device_set_rx_callback(uint8_t itf, usb_cdc_rx_cb cb, void *data)
{
    g_cdc_device[itf].rx_cb = cb;
    g_cdc_device[itf].rx_cb_data = data;
}

void usb_cdc_device_set_tx_complete_callback(uint8_t itf, usb_cdc_tx_complete_cb cb, void *data)
{
    g_cdc_device[itf].tx_complete_cb = cb;
    g_cdc_device[itf].tx_complete_cb_data = data;
}

void usb_cdc_device_set_line_state_callback(uint8_t itf, usb_cdc_line_state_cb cb, void *data)
{
    g_cdc_device[itf].line_state_cb = cb;
    g_cdc_device[itf].line_state_cb_data = data;
}

void usb_cdc_device_set_line_coding_callback(uint8_t itf, usb_cdc_line_coding_cb cb, void *data)
{
    g_cdc_device[itf].line_coding_cb = cb;
    g_cdc_device[itf].line_coding_cb_data = data;
}

//--------------------------------------------------------------------+
// CDC Device callbacks
//--------------------------------------------------------------------+

void tud_cdc_rx_cb(uint8_t itf)
{
    if (g_cdc_device[itf].rx_cb) {
        g_cdc_device[itf].rx_cb(itf, g_cdc_device[itf].rx_cb_data);
    }
}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    if (g_cdc_device[itf].tx_complete_cb) {
        g_cdc_device[itf].tx_complete_cb(itf, g_cdc_device[itf].tx_complete_cb_data);
    }
}


// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    if (g_cdc_device[itf].line_state_cb) {
        g_cdc_device[itf].line_state_cb(itf, dtr, rts, g_cdc_device[itf].line_state_cb_data);
    }
}



void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *line_coding)
{
    if (g_cdc_device[itf].line_coding_cb) {
        g_cdc_device[itf].line_coding_cb(itf, line_coding, g_cdc_device[itf].line_coding_cb_data);
    }
}


//--------------------------------------------------------------------+
// HID Device callbacks
//--------------------------------------------------------------------+


