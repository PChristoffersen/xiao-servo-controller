#include "lamp_array.h"

#include <tusb.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "board_config.h"
#include "lamp_array_descriptors.h"
#include "../lighting/neopixel.h"

#if 0
#define DEBUG(X...) printf(X)
#else
#define DEBUG(X...)
#endif


#define LAMP_COUNT (NEOPIXEL_STRIP1_COUNT)

#define LAMP_MIN_UPDATE_INTERVAL 20000 // 20ms
#define LAMP_UPDATE_LATENCY 10000 // 10ms

#define LAMP_WIDTH 10000
#define LAMP_HEIGHT 10000
#define LAMP_DEPTH 2000

static const tud_desc_lamp_array_attributes_report_t array_atttibutes_report = {
    .lamp_count = LAMP_COUNT,
    .width = LAMP_COUNT*LAMP_WIDTH,
    .height = LAMP_HEIGHT,
    .depth = LAMP_DEPTH,
    .kind = LAMP_ARRAY_KIND_CHASSIS,
    .min_update_interval = LAMP_MIN_UPDATE_INTERVAL
};


static uint16_t current_lamp_id = 0;
static bool autonomous_mode = false;

uint16_t lamp_array_get_array_attributes(uint8_t* buffer, uint16_t reqlen)
{
    if (reqlen < sizeof(array_atttibutes_report)) {
        DEBUG("lamp_array_get_array_attributes: invalid buffer size\n");
        return 0;
    }
    DEBUG("lamp_array_get_array_attributes report_size=%z\n", sizeof(array_atttibutes_report));
    assert(reqlen >= sizeof(array_atttibutes_report));
    memcpy(buffer, &array_atttibutes_report, sizeof(array_atttibutes_report));
    return sizeof(array_atttibutes_report);
}

void lamp_array_set_attributes(uint8_t const* buffer, uint16_t bufsize)
{
    if (bufsize != sizeof(tud_desc_lamp_attributes_request_t)) {
        DEBUG("lamp_array_set_attributes: invalid buffer size\n");
        return;
    }
    const tud_desc_lamp_attributes_request_t* request = (const tud_desc_lamp_attributes_request_t*) buffer;

    DEBUG("lamp_array_set_attributes: lamp_id=%u\n", request->lamp_id);
    if (request->lamp_id >= LAMP_COUNT) {
        DEBUG("lamp_array_set_attributes: invalid lamp_id\n");
        return;
    }
    current_lamp_id = request->lamp_id;
}



uint16_t lamp_array_get_attributes(uint8_t* buffer, uint16_t reqlen)
{
    if (reqlen < sizeof(tud_desc_lamp_attributes_response_t)) {
        DEBUG("lamp_array_get_attributes: invalid buffer size\n");
        return 0;
    }
    DEBUG("lamp_array_get_attributes: lamp_id=%u\n", current_lamp_id);

    tud_desc_lamp_attributes_response_t report = {
        .lamp_id = current_lamp_id,
        .position_x = LAMP_WIDTH * current_lamp_id + LAMP_WIDTH/2,
        .position_y = LAMP_HEIGHT/2,
        .position_z = 0,
        .purpose = LAMP_PURPOSE_BRANDING,
        .update_latency = LAMP_UPDATE_LATENCY,
        .red_level_count = 0xFF,
        .green_level_count = 0xFF,
        .blue_level_count = 0xFF,
        .intensity_level_count = 1,
        .is_programmable = 1
    };

    if (current_lamp_id+1 < array_atttibutes_report.lamp_count) {
        current_lamp_id++;
    }
    else {
        current_lamp_id = 0;
    }

    memcpy(buffer, &report, sizeof(report));
    
    return sizeof(report);
}




void lamp_array_set_multi_update(uint8_t const* buffer, uint16_t bufsize)
{
    if (bufsize != sizeof(tud_desc_lamp_multi_update_t)) {
        DEBUG("lamp_array_set_multi_update: invalid buffer size %u\n", bufsize);
        return;
    }
    
    tud_desc_lamp_multi_update_t const* update = (tud_desc_lamp_multi_update_t const*) buffer;
    DEBUG("lamp_array_set_multi_update: lamp_count=%u flags=%02x\n", update->lamp_count, update->flags);

    if (!autonomous_mode) {
        for (uint16_t i=0; i<update->lamp_count; i++) {
            if (update->lamp_ids[i] >= LAMP_COUNT) {
                DEBUG("lamp_array_set_multi_update: invalid lamp_id\n");
                return;
            }
            neopixel_strip_set(NEOPIXEL_STRIP1, update->lamp_ids[i], ugrb_color(update->color[i].red, update->color[i].green, update->color[i].blue));
        }
        if (update->flags & LAMP_ARRAY_FLAG_UPDATE_COMPLETE) {
            neopixel_strip_show(NEOPIXEL_STRIP1);
        }
    }
}


void lamp_array_set_range_update(uint8_t const* buffer, uint16_t bufsize)
{
    if (bufsize != sizeof(tud_desc_lamp_range_update_t)) {
        DEBUG("lamp_array_set_range_update: invalid buffer size\n");
        return;
    }
    tud_desc_lamp_range_update_t const* update = (tud_desc_lamp_range_update_t const*) buffer;
    
    if (update->lamp_id_start >= LAMP_COUNT || update->lamp_id_end >= LAMP_COUNT) {
        DEBUG("lamp_array_set_range_update: invalid lamp_id\n");
        return;
    }
    
    DEBUG("lamp_array_set_range_update: flags=%02x lamp_id_start=%u lamp_id_end=%u color=%02X%02X%02X%02X\n", update->flags, update->lamp_id_start, update->lamp_id_end, update->color.red, update->color.green, update->color.blue, update->color.intensity);

    if (!autonomous_mode) {
        for (uint16_t i=update->lamp_id_start; i<=update->lamp_id_end; i++) {
            neopixel_strip_set(NEOPIXEL_STRIP1, i, ugrb_color(update->color.red, update->color.green, update->color.blue));
        }
        if (update->flags & LAMP_ARRAY_FLAG_UPDATE_COMPLETE) {
            neopixel_strip_show(NEOPIXEL_STRIP1);
        }
    }

}


void lamp_array_set_array_control(uint8_t const* buffer, uint16_t bufsize)
{
    assert(bufsize == sizeof(tud_desc_lamp_control_t));
    tud_desc_lamp_control_t const* control = (tud_desc_lamp_control_t const*) buffer;

    DEBUG("lamp_array_set_array_control: autonomousMode=%u\n", control->autonomousMode);
    bool new_mode = control->autonomousMode;
    if (new_mode != autonomous_mode) {
        neopixel_strip_fill(NEOPIXEL_STRIP1, NEOPIXEL_BLACK);
        neopixel_strip_show(NEOPIXEL_STRIP1);
        autonomous_mode = new_mode;
    }
}




