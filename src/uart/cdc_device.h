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

#pragma once

#include <pico/stdlib.h>
#include <tusb.h>

typedef void (*usb_cdc_rx_cb)(uint8_t itf, void *data);
typedef void (*usb_cdc_tx_complete_cb)(uint8_t itf, void *data);
typedef void (*usb_cdc_line_state_cb)(uint8_t itf, bool dtr, bool rts, void *data);
typedef void (*usb_cdc_line_coding_cb)(uint8_t itf, cdc_line_coding_t const *line_coding, void *data);


void usb_device_init();

void usb_cdc_device_set_rx_callback(uint8_t itf, usb_cdc_rx_cb cb, void *data);
void usb_cdc_device_set_tx_complete_callback(uint8_t itf, usb_cdc_tx_complete_cb cb, void *data);
void usb_cdc_device_set_line_state_callback(uint8_t itf, usb_cdc_line_state_cb cb, void *data);
void usb_cdc_device_set_line_coding_callback(uint8_t itf, usb_cdc_line_coding_cb cb, void *data);
