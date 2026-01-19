#pragma once

#include <driver/media_types.h>

media_rotate_t get_string_to_angle(char *string);
char * get_string_to_lcd_name(char *string);
media_ppi_t get_string_to_ppi(char *string);
uint32_t get_ppi_size(media_ppi_t ppi);
uint32_t get_current_timestamp(void);
media_ppi_cap_t pixel_ppi_to_cap(media_ppi_t ppi);
void h264_encode_sei_init(uint8_t *sei);
void *media_malloc(uint32_t size);

#define UNUSED_ATTR __attribute__((unused))
