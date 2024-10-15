// Copyright 2020-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/psram_types.h>
#include <driver/media_types.h>

#include "bk_list.h"

#define GPIO_DVP_HSYNC		(30)
#define GPIO_DVP_VSYNC		(31)
#define GPIO_DVP_PCLK		(29)
#define GPIO_DVP_MCLK		(27)

#define GPIO_DVP_D0		(32)
#define GPIO_DVP_D1		(33)
#define GPIO_DVP_D2		(34)
#define GPIO_DVP_D3		(35)
#define GPIO_DVP_D4		(36)
#define GPIO_DVP_D5		(37)
#define GPIO_DVP_D6		(38)
#define GPIO_DVP_D7		(39)

//#define PIPELINE_DIAG_DEBUG

#define PIPELINE_ROTATE_CONTINUE 	(1)
#define PIPELINE_DECODE_LINE        (16)  /**< once decode 16 lines */

#ifdef CONFIG_SUPPORTED_IMAGE_MAX_720P
#define SUPPORTED_IMAGE_MAX_720P (1)
#endif

#ifdef PIPELINE_DIAG_DEBUG
#define ENCODE_DIAG_DEBUG
#define DECODE_DIAG_DEBUG
#define ROTATE_DIAG_DEBUG
#define SCALE_DIAG_DEBUG
//#define DISP_DIAG_DEBUG
#endif

typedef enum
{
	PIPELINE_MOD_H264,
	PIPELINE_MOD_ROTATE,
	PIPELINE_MOD_SCALE,
	PIPELINE_MOD_LINE_MAX,
	PIPELINE_MOD_SW_DEC = PIPELINE_MOD_LINE_MAX,
	PIPELINE_MOD_MAX,
} pipeline_module_t;

typedef struct {
	uint32_t frame_start : 1;
	uint32_t decoder_line : 1;
	uint32_t rotate_line : 1;
	uint32_t dma2d_line : 1;
	uint32_t h264_line : 1;
} mux_debug_t;

typedef enum
{
	MUX_BUFFER_IDLE = 0,
	MUX_BUFFER_PRESET,
	MUX_BUFFER_SHAREED,
	MUX_BUFFER_RELEASE,
} mux_buffer_state_t;

typedef enum
{
    MUX_DEC_ERR = 0,
	MUX_DEC_OK,
	MUX_DEC_TIMEOUT,
} dec_result_t;

typedef struct {

	union {
		frame_buffer_t *frame_buffer;
		uint8_t *data;
	};

	uint8_t index;
	uint8_t id;

	uint8_t state;
	uint8_t ok;

} complex_buffer_t;


typedef struct {
	mux_buffer_state_t state[PIPELINE_MOD_MAX];
	complex_buffer_t buffer;
	uint8_t encoded : 1;
} pipeline_mux_buf_t;


typedef struct {
	uint8_t jdec_type : 1;	 // by line(0) or by complete frame(1)
	uint16_t width;
	uint16_t height;
	pixel_format_t fmt;
	complex_buffer_t *buffer;
	LIST_HEADER_T list;
} pipeline_encode_request_t;


typedef bk_err_t (*mux_callback_t)(void *param);
typedef bk_err_t (*mux_request_callback_t)(pipeline_encode_request_t *request, mux_callback_t cb);
typedef bk_err_t (*mux_reset_callback_t)(mux_callback_t reset_cb);

typedef struct
{
	LIST_HEADER_T list;
	uint8_t frame_busy : 1;
    frame_buffer_t *frame;
} jpeg_decode_list_t;

#define IMAGE_MAX_PIPELINE_LINE		(16)
#define DISPLAY_MAX_PIPELINE_LINE	(16)
#define IMAGE_PIPEL_SIZE			(2)

#if SUPPORTED_IMAGE_MAX_720P

#define IMAGE_MAX_WIDTH				(1280)
#define IMAGE_MAX_HEIGHT			(720)

#define DISPLAY_MAX_WIDTH			(864)
#define DISPLAY_MAX_HEIGHT			(480)

#define DECODE_MAX_PIPELINE_LINE_SIZE	(IMAGE_MAX_WIDTH * IMAGE_MAX_PIPELINE_LINE * IMAGE_PIPEL_SIZE)
#define SCALE_MAX_PIPELINE_LINE_SIZE	(DISPLAY_MAX_WIDTH * IMAGE_MAX_PIPELINE_LINE * IMAGE_PIPEL_SIZE)
#define ROTATE_MAX_PIPELINE_LINE_SIZE	(DISPLAY_MAX_WIDTH * IMAGE_MAX_PIPELINE_LINE * IMAGE_PIPEL_SIZE)

#else

#define IMAGE_MAX_WIDTH				(864)
#define IMAGE_MAX_HEIGHT			(480)

#define DISPLAY_MAX_WIDTH			(864)
#define DISPLAY_MAX_HEIGHT			(480)

#define DECODE_MAX_PIPELINE_LINE_SIZE	(IMAGE_MAX_WIDTH * IMAGE_MAX_PIPELINE_LINE * IMAGE_PIPEL_SIZE)
#define ROTATE_MAX_PIPELINE_LINE_SIZE	(IMAGE_MAX_WIDTH * IMAGE_MAX_PIPELINE_LINE * IMAGE_PIPEL_SIZE)

#endif


typedef struct {
#if SUPPORTED_IMAGE_MAX_720P
	uint8_t decoder[DECODE_MAX_PIPELINE_LINE_SIZE * 2];
	uint8_t scale[SCALE_MAX_PIPELINE_LINE_SIZE * 2];
	uint8_t rotate[ROTATE_MAX_PIPELINE_LINE_SIZE * 2];
#else
	uint8_t decoder[DECODE_MAX_PIPELINE_LINE_SIZE * 2];
	uint8_t rotate[ROTATE_MAX_PIPELINE_LINE_SIZE * 2];
#endif
} mux_sram_buffer_t;

extern mux_sram_buffer_t *mux_sram_buffer;

void decoder_mux_dump(void);

bk_err_t bk_h264_encode_request(pipeline_encode_request_t *request, mux_callback_t cb);
bk_err_t bk_rotate_encode_request(pipeline_encode_request_t *request, mux_callback_t cb);
bk_err_t bk_scale_encode_request(pipeline_encode_request_t *request, mux_callback_t cb);
void bk_jdec_buffer_request_register(pipeline_module_t module, mux_request_callback_t cb, mux_reset_callback_t reset_cb);
void bk_jdec_buffer_request_deregister(pipeline_module_t module);
bk_err_t bk_scale_pipeline_init(void);
bk_err_t bk_rotate_pipeline_init(void);
bk_err_t bk_jdec_pipeline_init(void);
bk_err_t bk_h264_pipeline_init(void);

bk_err_t bk_h264_reset_request(mux_callback_t cb);
bk_err_t bk_scale_reset_request(mux_callback_t cb);
bk_err_t bk_rotate_reset_request(mux_callback_t cb);

void decoder_mux_dump(void);

#ifdef __cplusplus
}
#endif

