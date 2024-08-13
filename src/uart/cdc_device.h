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
