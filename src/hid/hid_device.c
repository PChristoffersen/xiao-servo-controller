#include "hid_device.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "usb_descriptors.h"
#include "lamp_array.h"

//--------------------------------------------------------------------+
// HID Device callbacks
//--------------------------------------------------------------------+

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    switch (report_id) {
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
