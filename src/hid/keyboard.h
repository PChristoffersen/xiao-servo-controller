#pragma once

#include <pico/stdlib.h>

void keyboard_init();

void keyboard_set_report(uint8_t const* buffer, uint16_t bufsize);
