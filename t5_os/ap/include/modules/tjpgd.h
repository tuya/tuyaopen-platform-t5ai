/*----------------------------------------------------------------------------/
/ TJpgDec - Tiny JPEG Decompressor R0.03 include file         (C)ChaN, 2021
/----------------------------------------------------------------------------*/

#pragma once

#include "driver/media_types.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void* jpeg_dec_handle_t; 

typedef enum{
	JD_FORMAT_RGB888 = 0,
	JD_FORMAT_RGB565 = 1,
	JD_FORMAT_Grayscale = 2,
	JD_FORMAT_YUYV,
	JD_FORMAT_VYUY,
	JD_FORMAT_VUYY,
}JD_FORMAT_OUTPUT;
/* Specifies output pixel format.
/  0: RGB888 (24-bit/pix)
/  1: RGB565 (16-bit/pix)
/  2: Grayscale (8-bit/pix)
/  3: VYUY (16-bit/pix)
/  4: YUYV (16-bit/pix)
*/

typedef enum{
	JD_LITTLE_ENDIAN = 0,
	JD_BIG_ENDIAN
} JD_BYTE_ORDER;
/* Specifies output byte order
/  0: LITTLE_ENDIAN
/  1: BIG_ENDIAN
*/

typedef enum{
	JD_NO_BUFFER_COPY = 0,
	JD_SINGLE_BUFFER_COPY,
	JD_DOUBLE_BUFFER_COPY,
} JD_COPY_TYPE_T;


typedef struct{
	JD_FORMAT_OUTPUT format;
	JD_BYTE_ORDER byte_order;
	uint8_t scale;
} jd_output_format;

typedef struct{
	uint8_t jpeg_dec[0xB0];
	uint8_t config[0x60];
	uint8_t work_buf[10240];
} jd_workbuf_t;

void jd_set_output_format_by_handle(jpeg_dec_handle_t handle, jd_output_format *output_format);
void jd_set_format_by_handle(jpeg_dec_handle_t handle, JD_FORMAT_OUTPUT format);
JD_FORMAT_OUTPUT jd_get_format_by_handle(jpeg_dec_handle_t handle);
void jd_set_scale_by_handle(jpeg_dec_handle_t handle, uint8_t scale);
int jd_get_scale_by_handle(jpeg_dec_handle_t handle);
void jd_set_byte_order_by_handle(jpeg_dec_handle_t handle, JD_BYTE_ORDER byte_order);
JD_BYTE_ORDER jd_get_byte_order_by_handle(jpeg_dec_handle_t handle);
media_rotate_t jd_get_rotate_by_hanlde(jpeg_dec_handle_t handle);
void jd_set_rotate_by_handle(jpeg_dec_handle_t handle, media_rotate_t rotate_angle, uint8_t *rotate_buf);

void jd_set_jpg_copy_func_by_handle(jpeg_dec_handle_t handle, uint8_t *copy_buffer_1, uint8_t *copy_buffer_2, uint32_t copy_buffer_size, void *jpg_copy_func, JD_COPY_TYPE_T type);

#ifdef __cplusplus
}
#endif
