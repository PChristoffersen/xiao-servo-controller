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

#define INVALID_PIN UINT32_MAX

/* Pin aliases */
#define XIAO_D0_PIN  26u
#define XIAO_D1_PIN  27u
#define XIAO_D2_PIN  28u
#define XIAO_D3_PIN  29u
#define XIAO_D4_PIN   6u
#define XIAO_D5_PIN   7u
#define XIAO_D6_PIN   0u
#define XIAO_D7_PIN   1u
#define XIAO_D8_PIN   2u
#define XIAO_D9_PIN   4u
#define XIAO_D10_PIN  3u


#define XIAO_LED_R_PIN 17u
#define XIAO_LED_G_PIN 16u
#define XIAO_LED_B_PIN 25u

#define XIAO_NEOPIXEL_PIN       PICO_DEFAULT_WS2812_PIN
#define XIAO_NEOPIXEL_POWER_PIN PICO_DEFAULT_WS2812_POWER_PIN
