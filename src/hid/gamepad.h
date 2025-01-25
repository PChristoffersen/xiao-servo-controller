#pragma once

#include <pico/stdlib.h>

void gamepad_init();

void gamepad_set_report(uint8_t const* buffer, uint16_t bufsize);
