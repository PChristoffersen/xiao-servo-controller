#pragma once

#include <pico/stdlib.h>

uint16_t lamp_array_get_array_attributes(uint8_t* buffer, uint16_t reqlen);
uint16_t lamp_array_get_attributes(uint8_t* buffer, uint16_t reqlen);

void lamp_array_set_attributes(uint8_t const* buffer, uint16_t bufsize);
void lamp_array_set_multi_update(uint8_t const* buffer, uint16_t bufsize);
void lamp_array_set_range_update(uint8_t const* buffer, uint16_t bufsize);
void lamp_array_set_array_control(uint8_t const* buffer, uint16_t bufsize);