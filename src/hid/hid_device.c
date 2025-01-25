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

#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "usb_descriptors.h"
#include "lamp_array.h"
#include "gamepad.h"

//--------------------------------------------------------------------+
// HID Device callbacks
//--------------------------------------------------------------------+

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    switch (report_id) {
        case REPORT_ID_GAMEPAD:
            gamepad_set_report(buffer, bufsize);
            break;
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST: 
            lamp_array_set_attributes(buffer, bufsize);
            break;
        case REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE: 
            lamp_array_set_multi_update(buffer, bufsize);
            break;   
        case REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE: 
            lamp_array_set_range_update(buffer, bufsize);
            break;   
        case REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL: 
            lamp_array_set_array_control(buffer, bufsize);
            break;
        default: 
            printf("HID Set report: report_id=%d buffer_sz=%u\n", report_id, bufsize);
            break;
    }
}


uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    switch (report_id) {
        case REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES: 
            return lamp_array_get_array_attributes(buffer, reqlen);
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE:
            return lamp_array_get_attributes(buffer, reqlen);
        default: 
            printf("HID Get report: report_id=%d  reqlen=%u\n", report_id, reqlen);
            break;
    }

    return 0;
}
