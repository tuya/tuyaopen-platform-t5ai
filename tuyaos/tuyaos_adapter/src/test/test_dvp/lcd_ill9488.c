#include "tuya_cloud_types.h"
#include "tal_display_service.h"

static const DISPLAY_INIT_SEQ_T ili9488_init_seq[] = {
    {.type = TY_INIT_REG,   .reg = {.r = 0xC0, .len = 2,  .v = {0x0E, 0x0E}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xC1, .len = 1,  .v = {0x46}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xC5, .len = 3,  .v = {0x00, 0x2D, 0x80}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB0, .len = 1,  .v = {0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB1, .len = 1,  .v = {0xA0}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB4, .len = 1,  .v = {0x02}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB5, .len = 4,  .v = {0x08, 0x0C, 0x50, 0x64}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xB6, .len = 2,  .v = {0x32, 0x02}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x36, .len = 1,  .v = {0x48}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x3A, .len = 1,  .v = {0x70}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x21, .len = 1,  .v = {0x00}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xE9, .len = 1,  .v = {0x01}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF7, .len = 4,  .v = {0xA9, 0x51, 0x2C, 0x82}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xF8, .len = 2,  .v = {0x21, 0x05}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xE0, .len = 15,  .v = {0x00, 0x0C, 0x10, 0x03, 0x0F, 0x05, 0x37, 0x66, 0x4D, 0x03, 0x0C, 0x0A, 0x2F, 0x35, 0x0F}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0xE1, .len = 15,  .v = {0x00, 0x0F, 0x16, 0x06, 0x13, 0x07, 0x3B, 0x35, 0x51, 0x07, 0x10, 0x0D, 0x36, 0x3B, 0x0F}}},
    {.type = TY_INIT_REG,   .reg = {.r = 0x11, .len = 0}},
    {.type = TY_INIT_DELAY, .delay_time = 120},
    {.type = TY_INIT_REG,   .reg = {.r = 0x29, .len = 0}},
    {.type = TY_INIT_DELAY, .delay_time = 20},
    {.type = TY_INIT_CONF_END}    //END
};

const ty_display_device_s lcd_rgb_ili9488_device = {
    .type = DISPLAY_RGB,
    .name = "ili9488", 
    .rgb = {
        .cfg = {
            .clk = 15000000,
            .out_data_clk_edge = RGB_DATA_IN_RISING_EDGE,
            .width = 320,
            .height = 480,
            .pixel_fmt = TY_PIXEL_FMT_RGB565,
            .hsync_back_porch = 80,
            .hsync_front_porch = 80,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .hsync_pulse_width = 20,
            .vsync_pulse_width = 4
        },
        .init_seq = ili9488_init_seq,
        .display_cfg = NULL
    }
};
