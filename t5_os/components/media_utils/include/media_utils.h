#pragma once

#include <driver/media_types.h>

media_rotate_t get_string_to_angle(char *string);
char * get_string_to_lcd_name(char *string);

#define UNUSED_ATTR __attribute__((unused))
