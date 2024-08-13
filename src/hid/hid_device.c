#include "hid_device.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include <tusb.h>


//--------------------------------------------------------------------+
// HID Device callbacks
//--------------------------------------------------------------------+

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    printf("HID Set report\n");
}


uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    printf("HID Get report\n");
    return 0;
}
