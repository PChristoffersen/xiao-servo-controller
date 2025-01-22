#include "hid_device.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// HID Device callbacks
//--------------------------------------------------------------------+

// TODO https://github.com/microsoft/RP2040MacropadHidSample

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    printf("HID Set report: report_id=%d buffer_sz=%u\n", report_id, bufsize);

    switch (report_id) {
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST: {
            printf("REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_REQUEST\n");
            //SetLampAttributesId(buffer);
            break;
        }
        case REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE: {
            printf("REPORT_ID_LIGHTING_LAMP_MULTI_UPDATE\n");
            //SetMultipleLamps(buffer);
            break;   
        }
        case REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE: {
            printf("REPORT_ID_LIGHTING_LAMP_RANGE_UPDATE\n");
            //SetLampRange(buffer);
            break;   
        }
        case REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL: {
            printf("REPORT_ID_LIGHTING_LAMP_ARRAY_CONTROL\n");
            //SetAutonomousMode(buffer);
            break;
        }
        default: {
            break;
        }
    }
}


uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    printf("HID Get report: report_id=%d  reqlen=%u\n", report_id, reqlen);

    switch (report_id) {
        case REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES: {
            printf("REPORT_ID_LIGHTING_LAMP_ARRAY_ATTRIBUTES\n");
            //return GetLampArrayAttributesReport(buffer);
            break;
        }
        case REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE: { 
            printf("REPORT_ID_LIGHTING_LAMP_ATTRIBUTES_RESPONSE\n");
            //return GetLampAttributesReport(buffer);
            break;
        }
        default: {
            break;
        }
    }

    return 0;
}
