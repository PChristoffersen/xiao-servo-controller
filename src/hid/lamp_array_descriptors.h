#pragma once

#include <tusb.h>

typedef enum {
    LAMP_ARRAY_KIND_UNDEFINED         = 0x00,
    LAMP_ARRAY_KIND_KEYBOARD          = 0x01,
    LAMP_ARRAY_KIND_MOUSE             = 0x02,
    LAMP_ARRAY_KIND_GAME_CONTROLLER   = 0x03,
    LAMP_ARRAY_KIND_PERIPHERAL        = 0x04,
    LAMP_ARRAY_KIND_SCENE             = 0x05,
    LAMP_ARRAY_KIND_NOTIFICATION      = 0x06,
    LAMP_ARRAY_KIND_CHASSIS           = 0x07,
    LAMP_ARRAY_KIND_WEARABLE          = 0x08,
    LAMP_ARRAY_KIND_FURNITURE         = 0x09,
    LAMP_ARRAY_KIND_ART               = 0x0A,
    LAMP_ARRAY_KIND_VENDOR_DEFINED_FIRST = 0x10000,
    LAMP_ARRAY_KIND_VENDOR_DEFINED_LAST = 0xFFFFFFFF
} tusb_desc_lamp_array_kind_t;

typedef enum {
    LAMP_PURPOSE_CONTROL      = 0x00,
    LAMP_PURPOSE_ACCENT       = 0x01,
    LAMP_PURPOSE_BRANDING     = 0x02,
    LAMP_PURPOSE_STATUS       = 0x03,
    LAMP_PURPOSE_ILLUMINATION = 0x04,
    LAMP_PURPOSE_PRESENTATION = 0x05,
    LAMP_PURPOSE_VENDOR_DEFINED_FIRST = 0x10000,
    LAMP_PURPOSE_VENDOR_DEFINED_LAST = 0xFFFFFFFF
} tusb_desc_lamp_purpose_t;


typedef enum {
    LAMP_ARRAY_FLAG_UPDATE_COMPLETE = (1U<<0),
} tusb_desc_lamp_array_flags_t;


typedef uint16_t tud_desc_lamp_id_t;

typedef struct TU_ATTR_PACKED {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t intensity;
} tud_desc_lamp_color_t;


typedef struct TU_ATTR_PACKED {
    uint16_t lamp_count; // Number of lamps associated with this array

    uint32_t width;  // Bounding box width in micrometers
    uint32_t height; // Bounding box height in micrometers
    uint32_t depth;  // Bounding box depth in micrometers

    uint32_t kind; // Kind of lamp array
    uint32_t min_update_interval; // minimal update interval in microseconds
} tud_desc_lamp_array_attributes_report_t;

typedef struct TU_ATTR_PACKED {
    uint16_t lamp_id;
} tud_desc_lamp_attributes_request_t;


typedef struct TU_ATTR_PACKED {
    tud_desc_lamp_id_t lamp_id;

    uint32_t position_x; // x position in micrometers relative to bounding box
    uint32_t position_y; // y position in micrometers relative to bounding box
    uint32_t position_z; // z position in micrometers relative to bounding box

    uint32_t update_latency; // update latency in microseconds
    uint32_t purpose; // Purpose of the lamp

    uint8_t red_level_count; // Number of red levels supported by the lamp
    uint8_t green_level_count; // Number of green levels supported by the lamp
    uint8_t blue_level_count; // Number of blue levels supported by the lamp
    uint8_t intensity_level_count; // Number of intensity levels supported by the lamp

    uint8_t is_programmable; // 1 if the lamp has programmble colors, 0 otherwise
    uint8_t input_binding; // Keyboard* Usages from the Keyboard/Keypad UsagePage (0x07), or Button* Usages from Button UsagePage (0x09)
} tud_desc_lamp_attributes_response_t;


typedef struct TU_ATTR_PACKED {
    uint8_t lamp_count;
    uint8_t flags;
    tud_desc_lamp_id_t lamp_ids[8];
    tud_desc_lamp_color_t color[8];
} tud_desc_lamp_multi_update_t;

typedef struct TU_ATTR_PACKED {
    uint8_t flags;
    tud_desc_lamp_id_t lamp_id_start;
    tud_desc_lamp_id_t lamp_id_end;
    tud_desc_lamp_color_t color;
} tud_desc_lamp_range_update_t;


typedef struct TU_ATTR_PACKED {
    uint8_t autonomousMode;
} tud_desc_lamp_control_t;