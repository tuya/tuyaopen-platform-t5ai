// Copyright 2020-2021 Beken
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

#include <driver/int.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include <driver/media_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/gpio_types.h>
#include <driver/gpio.h>
#include <driver/h264.h>
#include "bk_general_dma.h"
#include <driver/psram.h>
#include <driver/yuv_buf.h>
#include <driver/video_common_driver.h>
#include <driver/aon_rtc.h>

#include "bk_misc.h"
#include "gpio_driver.h"

#define TAG "dvp_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define H264_SPS_PPS_SIZE (41)
#define H264_SELF_DEFINE_SEI_SIZE (96)

static const uint8 crc8_table[256] =
{
	0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
	0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
	0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
	0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
	0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
	0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
	0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
	0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
	0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
	0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
	0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
	0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
	0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
	0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
	0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
	0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
	0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
	0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
	0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
	0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
	0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
	0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
	0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
	0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
	0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
	0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
	0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
	0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
	0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
	0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
	0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
	0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

static uint8 hnd_crc8(
    uint8 *pdata,   /* pointer to array of data to process */
    uint32  nbytes,   /* number of input data bytes to process */
    uint8 crc   /* either CRC8_INIT_VALUE or previous return value */
)
{
	/* hard code the crc loop instead of using CRC_INNER_LOOP macro
	 * to avoid the undefined and unnecessary (uint8 >> 8) operation.
	 */
	while (nbytes-- > 0)
	{
		crc = crc8_table[(crc ^ *pdata++) & 0xff];
	}

	return crc;
}


typedef struct
{
	uint8_t eof : 1;
	uint8_t error : 1;
	uint8_t index : 1;
	uint8_t cached : 1;
	uint8_t psram_dma_busy : 1;
	uint8_t sps_pps_flag : 1;
	uint8_t i_frame : 1;
	uint8_t regenerate_idr : 1;
	uint8_t sequence;

#ifndef CONFIG_H264_GOP_START_IDR_FRAME
	uint8_t sps_pps[64]; //for psram write through
#endif

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	uint8_t sei[H264_SELF_DEFINE_SEI_SIZE]; // save frame infomation
#endif

	uint8_t offset;
	uint8_t psram_dma;
	uint8 * buffer;
	uint32_t frame_id;
	uint32_t psram_dma_left;
	uint32_t dma_length;
	beken_semaphore_t sem;
	frame_buffer_t *frame;
} dvp_driver_t;

typedef struct
{
	uint32_t yuv_em_addr;
	uint32_t yuv_pingpong_length;
	uint32_t yuv_data_offset;
	uint8_t dma_collect_yuv;
}encode_yuv_config_t;

#define JPEG_CRC_SIZE (5)
#define FRAME_BUFFER_CACHE (1024 * 5)
#if !CONFIG_YUV_BUF
#define DVP_STRIP
#endif

//#define DVP_DIAG_DEBUG

#ifdef DVP_DIAG_DEBUG

#define DVP_DIAG_DEBUG_INIT()                   \
	do {                                        \
		gpio_dev_unmap(GPIO_2);                 \
		bk_gpio_disable_pull(GPIO_2);           \
		bk_gpio_enable_output(GPIO_2);          \
		bk_gpio_set_output_low(GPIO_2);         \
		\
		gpio_dev_unmap(GPIO_3);                 \
		bk_gpio_disable_pull(GPIO_3);           \
		bk_gpio_enable_output(GPIO_3);          \
		bk_gpio_set_output_low(GPIO_3);         \
		\
		gpio_dev_unmap(GPIO_4);                 \
		bk_gpio_disable_pull(GPIO_4);           \
		bk_gpio_enable_output(GPIO_4);          \
		bk_gpio_set_output_low(GPIO_4);         \
		\
		gpio_dev_unmap(GPIO_5);                 \
		bk_gpio_disable_pull(GPIO_5);           \
		bk_gpio_enable_output(GPIO_5);          \
		bk_gpio_set_output_low(GPIO_5);         \
		\
		gpio_dev_unmap(GPIO_12);                \
		bk_gpio_disable_pull(GPIO_12);          \
		bk_gpio_enable_output(GPIO_12);         \
		bk_gpio_set_output_low(GPIO_12);        \
		\
		gpio_dev_unmap(GPIO_13);                \
		bk_gpio_disable_pull(GPIO_13);          \
		bk_gpio_enable_output(GPIO_13);         \
		bk_gpio_set_output_low(GPIO_13);        \
		\
	} while (0)

#define DVP_JPEG_VSYNC_ENTRY()          bk_gpio_set_output_high(GPIO_2)
#define DVP_JPEG_VSYNC_OUT()            bk_gpio_set_output_low(GPIO_2)

#define DVP_JPEG_EOF_ENTRY()            bk_gpio_set_output_high(GPIO_3)
#define DVP_JPEG_EOF_OUT()              bk_gpio_set_output_low(GPIO_3)

#define DVP_YUV_EOF_ENTRY()             bk_gpio_set_output_high(GPIO_4)
#define DVP_YUV_EOF_OUT()               bk_gpio_set_output_low(GPIO_4)

#define DVP_JPEG_START_ENTRY()          bk_gpio_set_output_high(GPIO_5)
#define DVP_JPEG_START_OUT()            bk_gpio_set_output_low(GPIO_5)

#define DVP_JPEG_HEAD_ENTRY()           bk_gpio_set_output_high(GPIO_12)
#define DVP_JPEG_HEAD_OUT()             bk_gpio_set_output_low(GPIO_12)

#define DVP_PPI_ERROR_ENTRY()           DVP_YUV_EOF_ENTRY()
#define DVP_PPI_ERROR_OUT()             DVP_YUV_EOF_OUT()

#define DVP_H264_EOF_ENTRY()            DVP_JPEG_EOF_ENTRY()
#define DVP_H264_EOF_OUT()              DVP_JPEG_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()           bk_gpio_set_output_high(GPIO_13)
#define DVP_JPEG_SDMA_OUT()             bk_gpio_set_output_low(GPIO_13)

#define DVP_DEBUG_IO()                      \
	do {                                    \
		bk_gpio_set_output_high(GPIO_6);    \
		bk_gpio_set_output_low(GPIO_6);     \
	} while (0)

#else
#define DVP_DIAG_DEBUG_INIT()

#define DVP_JPEG_EOF_ENTRY()
#define DVP_JPEG_EOF_OUT()

#define DVP_YUV_EOF_ENTRY()
#define DVP_YUV_EOF_OUT()

#define DVP_JPEG_START_ENTRY()
#define DVP_JPEG_START_OUT()

#define DVP_JPEG_HEAD_ENTRY()
#define DVP_JPEG_HEAD_OUT()

#define DVP_PPI_ERROR_ENTRY()
#define DVP_PPI_ERROR_OUT()

#define DVP_H264_EOF_ENTRY()
#define DVP_H264_EOF_OUT()

#define DVP_JPEG_SDMA_ENTRY()
#define DVP_JPEG_SDMA_OUT()

#define DVP_JPEG_VSYNC_ENTRY()
#define DVP_JPEG_VSYNC_OUT()

#endif

extern media_debug_t *media_debug;

static dvp_camera_config_t *dvp_camera_config = NULL;
static const dvp_sensor_config_t *current_sensor = NULL;
static dvp_driver_t * dvp_camera_drv = NULL;
static media_state_t dvp_state = MASTER_TURN_OFF;
static uint8_t *dvp_camera_encode = NULL;
frame_buffer_t *curr_encode_frame = NULL;
frame_buffer_t *curr_yuv_frame = NULL;
jpeg_config_t jpeg_config = {0};

static uint8_t dvp_camera_dma_channel = DMA_ID_MAX;

static encode_yuv_config_t *encode_yuv_config = NULL;

bk_err_t dvp_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnls);


static const dvp_sensor_config_t **devices_list = NULL;
static uint16_t devices_size = 0;

const dvp_sensor_config_t **get_sensor_config_devices_list(void)
{
	return devices_list;
}

int get_sensor_config_devices_num(void)
{
	return devices_size;
}

void bk_dvp_camera_set_devices_list(const dvp_sensor_config_t **list, uint16_t size)
{
	devices_list = list;
	devices_size = size;
}

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
	uint32_t i;

	for (i = 0; i < devices_size; i++)
	{
		if (devices_list[i]->id == id)
		{
			return devices_list[i];
		}
	}

	return NULL;
}

const dvp_sensor_config_t *bk_dvp_get_sensor_auto_detect(void)
{
	uint32_t i;
	uint8_t count = 3;

	do {
		for (i = 0; i < devices_size; i++)
		{
			if (devices_list[i]->detect() == true)
			{
				return devices_list[i];
			}
		}

		count--;

		rtos_delay_milliseconds(5);

	} while (count > 0);

	return NULL;
}

static uint32_t dvp_camera_get_milliseconds(void)
{
	uint32_t time = 0;

#if CONFIG_ARCH_RISCV
	extern u64 riscv_get_mtimer(void);

	time = (riscv_get_mtimer() / 26000) & 0xFFFFFFFF;
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
	time = (bk_aon_rtc_get_us() / 1000) & 0xFFFFFFFF;
#endif

#endif

	return time;
}

#ifdef DVP_STRIP
static uint32_t dvp_frame_strip(uint8_t *src, uint32_t size)
{
	uint32_t i = 0;
	uint32_t first_zero = 0;
	uint32_t tail = 0, tail_old = 0;
	uint32_t length = 0;
	uint8_t sram_tmp[16] = {0};

	if ((size >> 3) & 0x1)
	{
		length = size & 0xFFFFFFF0;
		tail_old = tail = size & 0x0F;
	}
	else
	{
		length = (size & 0xFFFFFFF8) - 0x08;
		tail_old = tail = (size & 0x07) + 0x08;
	}

	for (i = 0; i < tail; i++)
	{
		sram_tmp[i] = src[length + i];
	}

	tail --;
	if (sram_tmp[tail] != 0xD9
		&& sram_tmp[tail - 1] != 0xFF)
	{
		LOGE("strip tail error %u \n", size);
		return 0;
	}
	else
	{
		tail -= 2;
	}

	for (i = tail; i > 0; i--)
	{
		if (sram_tmp[i] == 0xFF)
		{
			tail--;
		}
		else if(sram_tmp[i] == 0x00)
		{
			if (first_zero == 0)
			{
				if (sram_tmp[i-1] == 0xFF)
				{
					tail --;
					first_zero ++;
				}
				else if ((sram_tmp[i-1] & 0x01) && (sram_tmp[i-1] & 0x0F) != 0X0D)
				{
					tail--;
				}
				else
				{
					tail++;
					break;
				}
			}
			else
			{
				tail++;
				break;
			}
		}
		else
		{
			tail++;
			break;
		}
	}

	sram_tmp[tail++] = 0xFF;
	sram_tmp[tail++] = 0xD9;

	if (tail_old > tail)
	{
		os_memset(sram_tmp + tail, 0, tail_old - tail);
#if (CONFIG_PSRAM)
		bk_psram_word_memcpy(src + length, sram_tmp, 16);
#else
		for (i = 0; i < 16; i++)
		{
			src[length + i] = sram_tmp[i];
		}
#endif
	}

	return length + tail;
}
#endif

static bk_err_t dvp_camera_init_device(const dvp_sensor_config_t *sensor)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = 0;

	if (dvp_camera_config->device->info.resolution.width != (sensor->def_ppi >> 16) ||
		dvp_camera_config->device->info.resolution.height != (sensor->def_ppi & 0xFFFF))
	{
		if (!(pixel_ppi_to_cap((dvp_camera_config->device->info.resolution.width << 16)
			| dvp_camera_config->device->info.resolution.height) & (sensor->ppi_cap)))
		{
			dvp_camera_config->device->info.resolution.width = sensor->def_ppi >> 16;
			dvp_camera_config->device->info.resolution.height = sensor->def_ppi & 0xFFFF;
		}
	}

	return ret;
}

static void dvp_memcpy_finish_callback(dma_id_t id)
{
	if(dvp_state != MASTER_TURN_ON)
	{
		return;
	}

	dvp_camera_drv->psram_dma_busy = false;
	dvp_camera_drv->index = !dvp_camera_drv->index;

	if (dvp_camera_drv->eof == true)
	{
		frame_buffer_t *frame = dvp_camera_drv->frame;

		if (dvp_camera_drv->psram_dma_left != 0)
		{
			dvp_memcpy_by_chnl(frame->frame + frame->length,
			                   dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
			                   dvp_camera_drv->psram_dma_left, dvp_camera_drv->psram_dma);
			frame->length += dvp_camera_drv->psram_dma_left;
			frame->sequence = dvp_camera_drv->frame_id++;
			dvp_camera_drv->psram_dma_left = 0;
		}
		else
		{
			frame->length -= JPEG_CRC_SIZE;
#ifdef DVP_STRIP
			frame->length = dvp_frame_strip(frame->frame, frame->length);
			if (frame->length == 0)
			{
				dvp_camera_drv->index = 0;
				dvp_camera_drv->eof = false;
				if (dvp_state == MASTER_TURN_ON)
				{
					bk_dma_start(dvp_camera_dma_channel);
				}
				else
				{
					dvp_camera_config->fb_free(curr_encode_frame);
					curr_encode_frame = NULL;
				}
				return;
			}
#endif

			if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
			{
				media_debug->h264_length = frame->length;
				media_debug->h264_kbps += frame->length;
			}
			else
			{
				media_debug->jpeg_length = frame->length;
				media_debug->jpeg_kbps += frame->length;
			}

			dvp_camera_config->fb_complete(frame);

			dvp_camera_drv->index = 0;
			dvp_camera_drv->frame = NULL;
			dvp_camera_drv->eof = false;

			if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
			{
				curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
			}
			else // JPEG
			{
				curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
			}

			if (curr_encode_frame == NULL
				    || curr_encode_frame->frame == NULL)
			{
				LOGE("alloc frame error\n");
				return;
			}

			curr_encode_frame->width = dvp_camera_config->device->info.resolution.width;
			curr_encode_frame->height = dvp_camera_config->device->info.resolution.height;
			curr_encode_frame->fmt = dvp_camera_config->device->fmt;
			curr_encode_frame->type = dvp_camera_config->device->type;

			bk_dma_start(dvp_camera_dma_channel);
		}
	}
}

bk_err_t dvp_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnls)
{
	dma_config_t dma_config = {0};

	/* fix for psram 4bytes alignment */
	if (len % 4)
	{
		len = (len / 4 + 1) * 4;
	}

	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 1;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);

	dvp_camera_drv->psram_dma_busy = true;

	BK_LOG_ON_ERR(bk_dma_init(cpy_chnls, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnls, len));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnls, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnls, DMA_ATTR_SEC));
#endif
	BK_LOG_ON_ERR(bk_dma_register_isr(cpy_chnls, NULL, dvp_memcpy_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(cpy_chnls));
	BK_LOG_ON_ERR(bk_dma_start(cpy_chnls));
	//BK_WHILE (bk_dma_get_enable_status(cpy_chnl));

	return BK_OK;
}


static void dvp_camera_dma_finish_callback(dma_id_t id)
{
	DVP_JPEG_SDMA_ENTRY();

	if (dvp_camera_drv->cached == true)
	{
		if (curr_encode_frame == NULL
		    || curr_encode_frame->frame == NULL)
		{
			LOGE("%s curr_encode_frame NULL\n");
			return;
		}

		dvp_memcpy_by_chnl(curr_encode_frame->frame + curr_encode_frame->length,
							dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
							FRAME_BUFFER_CACHE, dvp_camera_drv->psram_dma);
		curr_encode_frame->length += FRAME_BUFFER_CACHE;
	}
	else
	{
		curr_encode_frame->length += FRAME_BUFFER_CACHE;
	}

	dvp_camera_drv->dma_length += FRAME_BUFFER_CACHE;

	DVP_JPEG_SDMA_OUT();
}

static bk_err_t dvp_camera_dma_config(void)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config = {0};
	uint32_t encode_fifo_addr;

	if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
	{
		curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	}
	else // MJPEG || MIX
	{
		curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
	}

	if (curr_encode_frame == NULL)
	{
		LOGE("malloc frame fail \r\n");
		ret = BK_FAIL;
		return ret;
	}

	curr_encode_frame->type = dvp_camera_config->device->type;
	curr_encode_frame->fmt = dvp_camera_config->device->fmt;
	curr_encode_frame->width = dvp_camera_config->device->info.resolution.width;
	curr_encode_frame->height = dvp_camera_config->device->info.resolution.height;

	if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
	{
#if (CONFIG_H264)
		bk_h264_get_fifo_addr(&encode_fifo_addr);
		curr_encode_frame->fmt = dvp_camera_config->device->fmt;
		//dvp_camera_dma_channel = bk_dma_alloc(DMA_DEV_H264);
		dvp_camera_dma_channel = bk_fixed_dma_alloc(DMA_DEV_H264, DMA_ID_8);
#endif
	}
	else // JPEG || MIX
	{
		bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
		curr_encode_frame->fmt = dvp_camera_config->device->fmt;
#if (CONFIG_SOC_BK7236XX)
		dvp_camera_dma_channel = bk_fixed_dma_alloc(DMA_DEV_JPEG, DMA_ID_8);
#else
		dvp_camera_dma_channel = bk_dma_alloc(DMA_DEV_JPEG);
#endif
	}

	if ((dvp_camera_dma_channel < DMA_ID_0) || (dvp_camera_dma_channel >= DMA_ID_MAX))
	{
		LOGE("malloc dma fail \r\n");
		ret = BK_FAIL;
		return ret;
	}
	LOGI("dvp_dma id:%d \r\n", dvp_camera_dma_channel);

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = encode_fifo_addr;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;

	if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
	{
		dma_config.src.dev = DMA_DEV_H264;
	}
	else // JPEG || MIX
	{
		dma_config.src.dev = DMA_DEV_JPEG;
	}

	// in bk7256 jpeg data cannot dma to psram direct, need copy to sram first, then copy to psram
	if (dvp_camera_drv->cached)
	{
		dma_config.dst.start_addr = (uint32_t)dvp_camera_drv->buffer;
		dma_config.dst.end_addr = (uint32_t)(dvp_camera_drv->buffer + FRAME_BUFFER_CACHE * 2);
		BK_LOG_ON_ERR(bk_dma_init(dvp_camera_dma_channel, &dma_config));
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_camera_dma_channel, FRAME_BUFFER_CACHE));
	}
	else
	{
		dma_config.dst.start_addr = (uint32_t)curr_encode_frame->frame;
		dma_config.dst.end_addr = (uint32_t)(curr_encode_frame->frame + curr_encode_frame->size);
		BK_LOG_ON_ERR(bk_dma_init(dvp_camera_dma_channel, &dma_config));
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_camera_dma_channel, FRAME_BUFFER_CACHE));
	}

	LOGI("%s, %d, start-stop:%p-%p\r\n", __func__, dvp_camera_drv->cached, dma_config.dst.start_addr, dma_config.dst.end_addr);

	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_camera_dma_channel, NULL, dvp_camera_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_camera_dma_channel));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_src_burst_len(dvp_camera_dma_channel, BURST_LEN_SINGLE));
	BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(dvp_camera_dma_channel, BURST_LEN_INC16));
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dvp_camera_dma_channel, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dvp_camera_dma_channel, DMA_ATTR_SEC));
#endif
	BK_LOG_ON_ERR(bk_dma_start(dvp_camera_dma_channel));

	return ret;
}

#if CONFIG_YUV_BUF
static bk_err_t encode_yuv_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};
	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 1;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);

	BK_LOG_ON_ERR(bk_dma_init(cpy_chnl, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnl, len));
#if (CONFIG_SPE && CONFIG_GDMA_HW_V2PX)
	BK_LOG_ON_ERR(bk_dma_set_src_burst_len(cpy_chnl, 3));
	BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(cpy_chnl, 3));
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC));
#endif

	return BK_OK;
}
#endif

static bk_err_t dvp_camera_init(yuv_mode_t mode)
{
	if (bk_dvp_camera_enumerate() == NULL)
	{
		return BK_FAIL;
	}

	/* set current used camera config */
	BK_RETURN_ON_ERR(dvp_camera_init_device(current_sensor));
	
	return BK_OK;
}

static bk_err_t dvp_camera_deinit(void)
{
	// step 1: deinit dvp gpio, data cannot transfer
	bk_video_gpio_deinit(DVP_GPIO_ALL);

	// step 2: deinit i2c
	bk_i2c_deinit(CONFIG_DVP_CAMERA_I2C_ID);

	// step 3: deinit hardware
#if (CONFIG_YUV_BUF)
	bk_yuv_buf_deinit();
	bk_h264_encode_disable();
	bk_h264_deinit();
#endif
	bk_jpeg_enc_deinit();

	bk_video_dvp_mclk_disable();

	// step 4: power off
	bk_video_power_off(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 1);

	// step 5: stop/deinit/free fifo to sram(psram) dma
	if (dvp_camera_config->device->mode != YUV_MODE && dvp_camera_config->device->mode != GRAY_MODE)
	{
		bk_dma_stop(dvp_camera_dma_channel);
		bk_dma_deinit(dvp_camera_dma_channel);
		if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
			bk_dma_free(DMA_DEV_H264, dvp_camera_dma_channel);
		else
			bk_dma_free(DMA_DEV_JPEG, dvp_camera_dma_channel);

		dvp_camera_dma_channel = DMA_ID_MAX;

#if CONFIG_YUV_BUF
		if (dvp_camera_config->device->mode == JPEG_YUV_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
		{
			if (encode_yuv_config)
			{
				bk_dma_stop(encode_yuv_config->dma_collect_yuv);
				bk_dma_deinit(encode_yuv_config->dma_collect_yuv);
				bk_dma_free(DMA_DEV_DTCM, encode_yuv_config->dma_collect_yuv);
			}

			os_free(encode_yuv_config);
			encode_yuv_config = NULL;
		}
#endif
	}

	// step 6: stop/deinit/free free sram->psram dma, and free dvp_camera_drv
	if (dvp_camera_drv)
	{
		if (dvp_camera_drv->cached)
		{
			bk_dma_stop(dvp_camera_drv->psram_dma);
			bk_dma_deinit(dvp_camera_drv->psram_dma);
			bk_dma_free(DMA_DEV_DTCM, dvp_camera_drv->psram_dma);

			if (dvp_camera_drv->buffer)
			{
				os_free(dvp_camera_drv->buffer);
				dvp_camera_drv->buffer = NULL;
			}
		}

		if (dvp_camera_drv->sem)
		{
			rtos_deinit_semaphore(&dvp_camera_drv->sem);
			dvp_camera_drv->sem = NULL;
		}

		os_free(dvp_camera_drv);
		dvp_camera_drv = NULL;
	}

	// step 7: free frame_buffer
	if (curr_encode_frame)
	{
		dvp_camera_config->fb_free(curr_encode_frame);
		curr_encode_frame = NULL;

		if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
		{
			dvp_camera_config->fb_clear(FB_INDEX_H264);
		}
		else
		{
			dvp_camera_config->fb_clear(FB_INDEX_JPEG);
		}
	}

	if (curr_yuv_frame)
	{
		dvp_camera_config->fb_free(curr_yuv_frame);
	}

	// step 8: free dvp_camera_config
	if (dvp_camera_config)
	{
		if (dvp_camera_config->device)
		{
			os_free(dvp_camera_config->device);
			dvp_camera_config->device = NULL;
		}

		os_free(dvp_camera_config);
		dvp_camera_config = NULL;
	}

	// step 9: free enode buffer
#if (!CONFIG_ENCODE_BUF_NOT_FREE)
	if (dvp_camera_encode)
	{
		os_free(dvp_camera_encode);
		dvp_camera_encode = NULL;
	}
#endif

	LOGI("%s complete!\r\n", __func__);

	dvp_state = MASTER_TURN_OFF;

	return BK_OK;
}

#if (CONFIG_YUV_BUF)
static void dvp_camera_reset_hardware_modules_handler(void)
{
	if (dvp_camera_config->device->mode == JPEG_MODE || dvp_camera_config->device->mode == JPEG_YUV_MODE)
	{
		bk_jpeg_enc_soft_reset();
	}

	if (dvp_camera_config->device->mode == H264_MODE || dvp_camera_config->device->mode == H264_YUV_MODE)
	{
		bk_h264_soft_reset();
	}

	bk_yuv_buf_soft_reset();

	if (dvp_camera_dma_channel < DMA_ID_MAX)
	{
		bk_dma_stop(dvp_camera_dma_channel);
		if (curr_encode_frame)
		{
			curr_encode_frame->length = 0;
		}
		bk_dma_start(dvp_camera_dma_channel);
	}
}

static void dvp_camera_sensor_ppi_err_handler(yuv_buf_unit_t id, void *param)
{
	DVP_PPI_ERROR_ENTRY();

	if (!dvp_camera_drv->error)
	{
		dvp_camera_drv->error = true;
	}

	DVP_PPI_ERROR_OUT();
}

static void yuv_sm0_line_done(void *param)
{
	if ((encode_yuv_config->yuv_data_offset + encode_yuv_config->yuv_pingpong_length) > curr_yuv_frame->length)
	{
		encode_yuv_config->yuv_data_offset = 0;
	}

	BK_WHILE(bk_dma_get_enable_status(encode_yuv_config->dma_collect_yuv));
	bk_dma_stop(encode_yuv_config->dma_collect_yuv);
	bk_dma_set_src_start_addr(encode_yuv_config->dma_collect_yuv,
							  (uint32_t)encode_yuv_config->yuv_em_addr);
	bk_dma_set_dest_start_addr(encode_yuv_config->dma_collect_yuv,
							   (uint32_t)(curr_yuv_frame->frame + encode_yuv_config->yuv_data_offset));
	bk_dma_start(encode_yuv_config->dma_collect_yuv);
	encode_yuv_config->yuv_data_offset += encode_yuv_config->yuv_pingpong_length;
}

static void yuv_sm1_line_done(void *param)
{
	if ((encode_yuv_config->yuv_data_offset + encode_yuv_config->yuv_pingpong_length) > curr_yuv_frame->length)
	{
		encode_yuv_config->yuv_data_offset = 0;
	}

	BK_WHILE(bk_dma_get_enable_status(encode_yuv_config->dma_collect_yuv));
	bk_dma_stop(encode_yuv_config->dma_collect_yuv);
	bk_dma_set_src_start_addr(encode_yuv_config->dma_collect_yuv,
							  (uint32_t)encode_yuv_config->yuv_em_addr + encode_yuv_config->yuv_pingpong_length);
	bk_dma_set_dest_start_addr(encode_yuv_config->dma_collect_yuv,
							   (uint32_t)(curr_yuv_frame->frame + encode_yuv_config->yuv_data_offset));
	bk_dma_start(encode_yuv_config->dma_collect_yuv);
	encode_yuv_config->yuv_data_offset += encode_yuv_config->yuv_pingpong_length;
}
#endif // bk7236xx

static void dvp_camera_vsync_negedge_handler(jpeg_unit_t id, void *param)
{
	DVP_JPEG_VSYNC_ENTRY();

	if (dvp_state == MASTER_TURNING_OFF)
	{
		bk_video_encode_stop(YUV_MODE);
		bk_video_encode_stop(JPEG_MODE);
		bk_video_encode_stop(H264_MODE);

		if (dvp_camera_drv->sem != NULL)
		{
			rtos_set_semaphore(&dvp_camera_drv->sem);
		}
		DVP_JPEG_VSYNC_OUT();
		return;
	}
#if (!CONFIG_YUV_BUF)
	else
	{
		if (dvp_camera_config->device->mode == YUV_MODE)
		{
			bk_video_encode_stop(YUV_MODE);
			bk_video_encode_start(YUV_MODE);
		}
	}
#else
	if (dvp_camera_drv->error)
	{
		dvp_camera_drv->error = false;
		dvp_camera_drv->sequence = 0;
		dvp_camera_reset_hardware_modules_handler();
		LOGI("reset OK \r\n");
		DVP_JPEG_VSYNC_OUT();
		return;
	}
#endif

	DVP_JPEG_VSYNC_OUT();
}

static void dvp_camera_yuv_eof_handler(jpeg_unit_t id, void *param)
{
	frame_buffer_t *frame = NULL;

	DVP_YUV_EOF_ENTRY();

	if (dvp_state != MASTER_TURN_ON)
	{
		DVP_YUV_EOF_OUT();
		return;
	}

	media_debug->isr_jpeg++;

	curr_yuv_frame->sequence = dvp_camera_drv->frame_id++;

	if (dvp_camera_config->device->mode == GRAY_MODE)
		curr_yuv_frame->length = dvp_camera_config->device->info.resolution.width * dvp_camera_config->device->info.resolution.height;
	else // YUV_MODE, and other carry yuv mode,jpeg_yuv/h264_yuv
		curr_yuv_frame->length = dvp_camera_config->device->info.resolution.width * dvp_camera_config->device->info.resolution.height * 2;

	dvp_camera_config->fb_complete(curr_yuv_frame);
	curr_yuv_frame = NULL;

#if !CONFIG_YUV_BUF
	if (dvp_camera_config->device->mode == JPEG_YUV_MODE)
	{
		bk_video_encode_stop(YUV_MODE);

		curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);

		if (curr_encode_frame == NULL
		    || curr_encode_frame->frame == NULL)
		{
			LOGE("alloc frame error\n");
			return;
		}

		curr_encode_frame->width = dvp_camera_config->device->info.resolution.width;
		curr_encode_frame->height = dvp_camera_config->device->info.resolution.height;
		curr_encode_frame->fmt = dvp_camera_config->device->fmt;
		curr_encode_frame->type = dvp_camera_config->device->type;

		bk_dma_start(dvp_camera_dma_channel);

		jpeg_config.mode = JPEG_MODE;
		bk_jpeg_enc_mode_switch(&jpeg_config);
		bk_jpeg_enc_start(JPEG_MODE);
	}
	else
#endif
	{
		uint32_t size = dvp_camera_config->device->info.resolution.width * dvp_camera_config->device->info.resolution.height * 2;
		frame = dvp_camera_config->fb_malloc(FB_INDEX_DISPLAY, size);

		if (frame != NULL)
		{
			curr_yuv_frame = frame;
			curr_yuv_frame->width = dvp_camera_config->device->info.resolution.width;
			curr_yuv_frame->height = dvp_camera_config->device->info.resolution.height;
			curr_yuv_frame->fmt = current_sensor->fmt;
			curr_yuv_frame->type = dvp_camera_config->device->type;
			bk_jpeg_set_em_base_addr(curr_yuv_frame->frame);
#if (CONFIG_YUV_BUF)
			bk_yuv_buf_set_em_base_addr((uint32_t)curr_yuv_frame->frame);
#endif
		}
		else
		{
			LOGE("%s malloc frame failed\n", __func__);
		}
	}

	DVP_YUV_EOF_OUT();
}

static void dvp_camera_jpeg_eof_handler(jpeg_unit_t id, void *param)
{
	DVP_JPEG_EOF_ENTRY();

	uint32_t real_length = 0, recv_length = 0;

	if (dvp_state != MASTER_TURN_ON)
	{
		DVP_JPEG_EOF_OUT();
		return;
	}

	if (dvp_camera_drv->error)
	{
		curr_encode_frame->length = 0;
		bk_dma_stop(dvp_camera_dma_channel);
		bk_dma_start(dvp_camera_dma_channel);
		DVP_JPEG_EOF_OUT();
		return;
	}

	if (curr_encode_frame == NULL
		|| curr_encode_frame->frame == NULL)
	{
		LOGE("curr_encode_frame NULL error\n");
		goto error;
	}

	media_debug->isr_jpeg++;

	bk_dma_flush_src_buffer(dvp_camera_dma_channel);

	real_length = bk_jpeg_enc_get_frame_size();

	recv_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(dvp_camera_dma_channel);

	bk_dma_stop(dvp_camera_dma_channel);

	dvp_camera_drv->dma_length += recv_length - JPEG_CRC_SIZE;

	if (dvp_camera_drv->dma_length != real_length)
	{
		LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, dvp_camera_drv->dma_length, real_length - dvp_camera_drv->dma_length);
	}

	dvp_camera_drv->dma_length = 0;

	if (dvp_camera_drv->cached)
	{
		if (dvp_camera_drv->psram_dma_busy == true)
		{
			media_debug->psram_busy++;
			dvp_camera_drv->frame = curr_encode_frame;
			dvp_camera_drv->eof = true;

			dvp_camera_drv->psram_dma_left = recv_length;
		}
		else
		{
			dvp_camera_drv->frame = curr_encode_frame;
			dvp_camera_drv->psram_dma_left = 0;
			dvp_camera_drv->eof = true;

			dvp_memcpy_by_chnl(curr_encode_frame->frame + curr_encode_frame->length,
								dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
								recv_length, dvp_camera_drv->psram_dma);
			curr_encode_frame->length += recv_length;
			curr_encode_frame->sequence = dvp_camera_drv->frame_id++;
		}
	}
	else
	{
		/*there not compare dma copy total length and read from jpeg register length, because register count is error*/
		if (dvp_camera_config->device->mode == JPEG_YUV_MODE)
		{
			curr_encode_frame->mix = true;
		}

		curr_encode_frame->length = curr_encode_frame->length + recv_length - JPEG_CRC_SIZE;

		if (curr_encode_frame->frame[curr_encode_frame->length - 2] == 0xFF
				&& curr_encode_frame->frame[curr_encode_frame->length - 1] == 0xD9)
		{
			dvp_camera_drv->eof = true;
		}
		else if (curr_encode_frame->frame[real_length - 2] == 0xFF
					&& curr_encode_frame->frame[real_length - 1] == 0xD9)
		{
			dvp_camera_drv->eof = true;
			curr_encode_frame->length = real_length;
		}
		else
		{
			dvp_camera_drv->eof = false;
		}

		if (dvp_camera_drv->eof)
		{
			media_debug->jpeg_length = curr_encode_frame->length;
			media_debug->jpeg_kbps += curr_encode_frame->length;
			curr_encode_frame->sequence = dvp_camera_drv->frame_id++;
			dvp_camera_config->fb_complete(curr_encode_frame);
			curr_encode_frame = dvp_camera_config->fb_malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
			if (curr_encode_frame == NULL
			|| curr_encode_frame->frame == NULL)
			{
				LOGE("alloc frame error\n");
				return;
			}
			curr_encode_frame->width = dvp_camera_config->device->info.resolution.width;
			curr_encode_frame->height = dvp_camera_config->device->info.resolution.height;
			curr_encode_frame->fmt = dvp_camera_config->device->fmt;
			curr_encode_frame->type = dvp_camera_config->device->type;
		}
		else
		{
			curr_encode_frame->length = 0;
		}

		dvp_camera_drv->eof = false;
		if (curr_encode_frame == NULL
		|| curr_encode_frame->frame == NULL)
		{
			LOGE("alloc frame error\n");
			return;
		}

		bk_dma_set_dest_addr(dvp_camera_dma_channel, (uint32_t)curr_encode_frame->frame, (uint32_t)(curr_encode_frame->frame + curr_encode_frame->size));
		bk_dma_start(dvp_camera_dma_channel);

		if (dvp_camera_config->device->mode == JPEG_YUV_MODE)
		{
			DVP_YUV_EOF_ENTRY();
			uint32_t size = dvp_camera_config->device->info.resolution.width * dvp_camera_config->device->info.resolution.height * 2;
			encode_yuv_config->yuv_data_offset = 0;
			bk_dma_flush_src_buffer(encode_yuv_config->dma_collect_yuv);
			curr_yuv_frame->sequence = dvp_camera_drv->frame_id;
			dvp_camera_config->fb_complete(curr_yuv_frame);
			curr_yuv_frame = dvp_camera_config->fb_malloc(FB_INDEX_DISPLAY, size);
			if (curr_yuv_frame != NULL)
			{
				curr_yuv_frame->width = dvp_camera_config->device->info.resolution.width;
				curr_yuv_frame->height = dvp_camera_config->device->info.resolution.height;
				curr_yuv_frame->fmt = current_sensor->fmt;
				curr_yuv_frame->type = dvp_camera_config->device->type;
				curr_yuv_frame->length = size;
			}
			else
			{
				LOGE("%s malloc frame failed\n", __func__);
			}
			DVP_YUV_EOF_OUT();
		}
	}

	DVP_JPEG_EOF_OUT();

	return;

error:
	dvp_camera_drv->index = 0;
	bk_dma_stop(dvp_camera_dma_channel);
	bk_video_encode_stop(JPEG_MODE);
}

#if (CONFIG_H264)
static void dvp_camera_h264_eof_handler(h264_unit_t id, void *param)
{
	uint32_t real_length = bk_h264_get_encode_count() * 4;
	uint32_t remain_length = 0;
	frame_buffer_t *new_frame = NULL;
#if (!CONFIG_H264_GOP_START_IDR_FRAME)
	uint8_t sps_pps_fill = 0;
#endif

	DVP_H264_EOF_ENTRY();

	dvp_camera_drv->sequence++;

	if (dvp_camera_drv->sequence > H264_GOP_FRAME_CNT)
	{
		dvp_camera_drv->sequence = 1;
	}


	if (dvp_camera_drv->sequence == 1)
	{
		dvp_camera_drv->i_frame = 1;
	}
	else
	{
		dvp_camera_drv->i_frame = 0;
	}

	if (dvp_state != MASTER_TURN_ON)
	{
		DVP_H264_EOF_OUT();
		return;
	}

	if (curr_encode_frame == NULL
		|| curr_encode_frame->frame == NULL)
	{
		LOGE("curr_encode_frame NULL error\n");
		DVP_H264_EOF_OUT();
		goto error;
	}

#if (CONFIG_H264_GOP_START_IDR_FRAME)
	if (dvp_camera_drv->sequence == H264_GOP_FRAME_CNT)
	{
		dvp_camera_drv->regenerate_idr = true;
	}
#endif

	if (real_length > CONFIG_H264_FRAME_SIZE - 0x20)
	{
		LOGE("%s size over h264 buffer range, %d\r\n", __func__, real_length);
		dvp_camera_drv->error = true;
	}

	bk_dma_flush_src_buffer(dvp_camera_dma_channel);

	remain_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(dvp_camera_dma_channel);

	bk_dma_stop(dvp_camera_dma_channel);

	dvp_camera_drv->dma_length += remain_length;

	if (dvp_camera_drv->dma_length != real_length)
	{
		uint32_t left_length = real_length - dvp_camera_drv->dma_length;
		LOGW("%s size no match:%d-%d=%d\r\n", __func__, real_length, dvp_camera_drv->dma_length, left_length);
		if (left_length != FRAME_BUFFER_CACHE)
		{
			dvp_camera_drv->error = true;
		}
	}

	dvp_camera_drv->dma_length = 0;

	if (dvp_camera_drv->error)
	{
		curr_encode_frame->length = 0;
		dvp_camera_drv->sps_pps_flag = false;
		dvp_camera_drv->sequence = 0;
		dvp_camera_drv->offset = 0;
		if (dvp_camera_config->device->mode == H264_YUV_MODE)
		{
			encode_yuv_config->yuv_data_offset = 0;
		}
		DVP_H264_EOF_OUT();
		return;
	}

	media_debug->isr_h264++;

	if (dvp_camera_drv->cached)
	{
		if (dvp_camera_drv->psram_dma_busy == true)
		{
			dvp_camera_drv->frame = curr_encode_frame;
			dvp_camera_drv->psram_dma_left = remain_length;
			dvp_camera_drv->eof = true;
			media_debug->psram_busy++;
		}
		else
		{
			dvp_camera_drv->frame = curr_encode_frame;
			dvp_camera_drv->psram_dma_left = 0;
			dvp_camera_drv->eof = true;

			dvp_memcpy_by_chnl(curr_encode_frame->frame + curr_encode_frame->length,
			                   dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
			                   remain_length, dvp_camera_drv->psram_dma);
			curr_encode_frame->length += remain_length;
			curr_encode_frame->sequence = dvp_camera_drv->frame_id++;
		}
	}
	else
	{
		if (dvp_camera_config->device->mode == H264_YUV_MODE)
		{
			curr_encode_frame->mix = true;
		}

		curr_encode_frame->length = real_length + dvp_camera_drv->offset;
		media_debug->h264_length = curr_encode_frame->length;
		media_debug->h264_kbps += curr_encode_frame->length;
		curr_encode_frame->sequence = dvp_camera_drv->frame_id++;

#if (!CONFIG_H264_GOP_START_IDR_FRAME)
		if (!dvp_camera_drv->sps_pps_flag)
		{
			if (curr_encode_frame->frame[4] == 0x67)
			{
				const uint8_t sei_fill_data[] = {
					0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0x0F, 0x01,
					0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01,
					0x02, 0x03, 0x04, 0x01, 0x05, 0x00, 0x80
				};

				os_memcpy(dvp_camera_drv->sps_pps, curr_encode_frame->frame, H264_SPS_PPS_SIZE);
				os_memcpy(&dvp_camera_drv->sps_pps[H264_SPS_PPS_SIZE], sei_fill_data, sizeof(dvp_camera_drv->sps_pps) - H264_SPS_PPS_SIZE);
				dvp_camera_drv->sps_pps_flag = 1;
				curr_encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS);
			}
			else
			{
				LOGE("%s sps pps head miss\n", __func__);
			}
		}
#endif
		if (dvp_camera_drv->i_frame)
		{
			curr_encode_frame->h264_type |= 1 << H264_NAL_I_FRAME;
#if (CONFIG_H264_GOP_START_IDR_FRAME)
			curr_encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS) | (1 << H264_NAL_IDR_SLICE);
#endif
		}
		else
		{
			curr_encode_frame->h264_type |= 1 << H264_NAL_P_FRAME;

#if (!CONFIG_H264_GOP_START_IDR_FRAME)
			if (dvp_camera_drv->sequence == (CONFIG_H264_P_FRAME_CNT + 1)
				&& dvp_camera_drv->sps_pps_flag)
			{
				sps_pps_fill = true;
			}
#endif
		}

		curr_encode_frame->timestamp = dvp_camera_get_milliseconds();

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
		curr_encode_frame->crc = hnd_crc8(curr_encode_frame->frame, curr_encode_frame->length, 0xFF);
		curr_encode_frame->length += H264_SELF_DEFINE_SEI_SIZE;
		os_memcpy(&dvp_camera_drv->sei[23], (uint8_t *)curr_encode_frame->frame, sizeof(frame_buffer_t));
		os_memcpy(&curr_encode_frame->frame[curr_encode_frame->length - H264_SELF_DEFINE_SEI_SIZE], &dvp_camera_drv->sei[0], H264_SELF_DEFINE_SEI_SIZE);
#endif

		LOGD("%s, I:%d, p:%d\r\n", __func__, (curr_encode_frame->h264_type & 0x1000020) > 0 ? 1 : 0, (curr_encode_frame->h264_type >> 23) & 0x1);

		new_frame = dvp_camera_config->fb_malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
		if (new_frame)
		{
			dvp_camera_config->fb_complete(curr_encode_frame);
			curr_encode_frame = new_frame;
		}
		else
		{
			dvp_camera_drv->regenerate_idr = true;
		}
		curr_encode_frame->width = dvp_camera_config->device->info.resolution.width;
		curr_encode_frame->height = dvp_camera_config->device->info.resolution.height;
		curr_encode_frame->length = 0;
		curr_encode_frame->fmt = dvp_camera_config->device->fmt;
		curr_encode_frame->type = dvp_camera_config->device->type;
		dvp_camera_drv->offset = 0;


#if (!CONFIG_H264_GOP_START_IDR_FRAME)
		if (sps_pps_fill)
		{
			dvp_camera_drv->offset = sizeof(dvp_camera_drv->sps_pps);
			os_memcpy(curr_encode_frame->frame, dvp_camera_drv->sps_pps, dvp_camera_drv->offset);
			curr_encode_frame->length += dvp_camera_drv->offset;
			curr_encode_frame->h264_type |= (1 << H264_NAL_SPS) | (1 << H264_NAL_PPS);
		}
#endif

		bk_dma_set_dest_addr(dvp_camera_dma_channel, (uint32_t)(curr_encode_frame->frame + dvp_camera_drv->offset), (uint32_t)(curr_encode_frame->frame + curr_encode_frame->size));
		bk_dma_start(dvp_camera_dma_channel);

		if (dvp_camera_config->device->mode == H264_YUV_MODE)
		{
			DVP_YUV_EOF_ENTRY();
			uint32_t size = dvp_camera_config->device->info.resolution.width * dvp_camera_config->device->info.resolution.height * 2;
			encode_yuv_config->yuv_data_offset = 0;
			bk_dma_flush_src_buffer(encode_yuv_config->dma_collect_yuv);
			curr_yuv_frame->sequence =  dvp_camera_drv->frame_id;
			dvp_camera_config->fb_complete(curr_yuv_frame);
			curr_yuv_frame = dvp_camera_config->fb_malloc(FB_INDEX_DISPLAY, size);
			if (curr_yuv_frame != NULL)
			{
				curr_yuv_frame->width = dvp_camera_config->device->info.resolution.width;
				curr_yuv_frame->height = dvp_camera_config->device->info.resolution.height;
				curr_yuv_frame->fmt = current_sensor->fmt;
				curr_yuv_frame->type = dvp_camera_config->device->type;
				curr_yuv_frame->length = size;
			}
			else
			{
				LOGE("%s malloc frame failed\n", __func__);
			}
			DVP_YUV_EOF_OUT();
		}
	}

	if (dvp_camera_drv->regenerate_idr)
	{
		dvp_camera_drv->regenerate_idr = false;
		dvp_camera_drv->sequence = 0;
		bk_h264_soft_reset();
	}

	DVP_H264_EOF_OUT();

	return;

error:
	dvp_camera_drv->index = 0;
	bk_dma_stop(dvp_camera_dma_channel);
	bk_video_encode_stop(H264_MODE);
	DVP_H264_EOF_OUT();
}
#endif // CONFIG_H264

static bk_err_t dvp_camera_jpeg_config_init(yuv_mode_t mode)
{
	int ret = BK_OK;

	if (mode == GRAY_MODE)
		jpeg_config.x_pixel = dvp_camera_config->device->info.resolution.width / 8 / 2;
	else
		jpeg_config.x_pixel = dvp_camera_config->device->info.resolution.width / 8;

	jpeg_config.y_pixel = dvp_camera_config->device->info.resolution.height / 8;
	jpeg_config.vsync = current_sensor->vsync;
	jpeg_config.hsync = current_sensor->hsync;
	jpeg_config.clk = current_sensor->clk;
	jpeg_config.mode = mode;

	switch (current_sensor->fmt)
	{
		case PIXEL_FMT_YUYV:
			jpeg_config.sensor_fmt = YUV_FORMAT_YUYV;
			break;

		case PIXEL_FMT_UYVY:
			jpeg_config.sensor_fmt = YUV_FORMAT_UYVY;
			break;

		case PIXEL_FMT_YYUV:
			jpeg_config.sensor_fmt = YUV_FORMAT_YYUV;
			break;

		case PIXEL_FMT_UVYY:
			jpeg_config.sensor_fmt = YUV_FORMAT_UVYY;
			break;

		default:
			LOGE("JPEG MODULE not support this sensor input format\r\n");
			ret = kParamErr;
			return ret;
	}

	ret = bk_jpeg_enc_init(&jpeg_config);
	if (ret != BK_OK)
	{
		LOGE("jpeg init error\n");
	}

	return ret;
}

#if CONFIG_YUV_BUF
bk_err_t dvp_camera_yuv_buf_config_init(yuv_mode_t mode)
{
	int ret = BK_OK;
	yuv_buf_config_t yuv_mode_config = {0};

	yuv_mode_config.work_mode = mode;
	yuv_mode_config.mclk_div = YUV_MCLK_DIV_3;

	if (mode != GRAY_MODE)
		yuv_mode_config.x_pixel = dvp_camera_config->device->info.resolution.width / 8;
	else
		yuv_mode_config.x_pixel = dvp_camera_config->device->info.resolution.width / 8 / 2;

	yuv_mode_config.y_pixel = dvp_camera_config->device->info.resolution.height / 8;
	yuv_mode_config.yuv_mode_cfg.vsync = current_sensor->vsync;
	yuv_mode_config.yuv_mode_cfg.hsync = current_sensor->hsync;

	LOGI("%s, %d-%d, mode:%d\r\n", __func__, dvp_camera_config->device->info.resolution.width, dvp_camera_config->device->info.resolution.height, mode);
	switch (current_sensor->fmt)
	{
		case PIXEL_FMT_YUYV:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;
			break;

		case PIXEL_FMT_UYVY:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UYVY;
			break;

		case PIXEL_FMT_YYUV:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YYUV;
			break;

		case PIXEL_FMT_UVYY:
			yuv_mode_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_UVYY;
			break;

		default:
			LOGE("YUV_BUF MODULE not support this sensor input format\r\n");
			ret = kParamErr;
	}

	if (ret != BK_OK)
	{
		return ret;
	}

	if (mode != GRAY_MODE && mode != YUV_MODE)
	{
		if (dvp_camera_encode == NULL)
		{
			dvp_camera_encode = dvp_camera_yuv_base_addr_init(dvp_camera_config->device->info.resolution, mode);
			if (dvp_camera_encode == NULL)
			{
				return BK_ERR_NO_MEM;
			}
		}

		LOGI("%s, encode_buf:0x%p\r\n", __func__, dvp_camera_encode);

		yuv_mode_config.base_addr = dvp_camera_encode;
	}

	ret = bk_yuv_buf_init(&yuv_mode_config);
	if (ret != BK_OK) {
		LOGE("yuv_buf yuv mode init error\n");
	}

	return ret;
}
#endif

static bk_err_t dvp_camera_yuv_mode(dvp_camera_config_t *config)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = BK_OK;
	uint32_t size = 0;

#if (CONFIG_YUV_BUF)
	ret = dvp_camera_yuv_buf_config_init(YUV_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}
#else
	ret = dvp_camera_jpeg_config_init(YUV_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}
#endif

	config->fb_init(FB_INDEX_DISPLAY);

	size = config->device->info.resolution.width * config->device->info.resolution.height * 2;

	curr_yuv_frame = dvp_camera_config->fb_malloc(FB_INDEX_DISPLAY, size);

	if (curr_yuv_frame == NULL)
	{
		LOGE("malloc frame fail \r\n");
		ret = BK_FAIL;
	}

	curr_yuv_frame->width = config->device->info.resolution.width;
	curr_yuv_frame->height = config->device->info.resolution.height;
	curr_yuv_frame->fmt = current_sensor->fmt;
	curr_yuv_frame->length = size;
	bk_jpeg_set_em_base_addr(curr_yuv_frame->frame);
#if (CONFIG_YUV_BUF)
	bk_yuv_buf_set_em_base_addr((uint32_t)curr_yuv_frame->frame);
#else
	bk_jpeg_set_em_base_addr(curr_yuv_frame->frame);
#endif

	return ret;
}

static bk_err_t dvp_camera_jpeg_mode(dvp_camera_config_t *config)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = BK_OK;

	config->fb_init(FB_INDEX_JPEG);

	ret = dvp_camera_dma_config();

	if (ret != BK_OK)
	{
		LOGE("dma init failed\n");
		return ret;
	}

#if (CONFIG_YUV_BUF)
	ret = dvp_camera_yuv_buf_config_init(JPEG_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}
#endif

	ret = dvp_camera_jpeg_config_init(JPEG_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}

#if (CONFIG_YUV_BUF)
	if (config->device->mode == JPEG_YUV_MODE)
	{
		uint32_t size = config->device->info.resolution.width * config->device->info.resolution.height * 2;
		config->fb_init(FB_INDEX_DISPLAY);
		curr_yuv_frame = config->fb_malloc(FB_INDEX_DISPLAY, size);
		if(curr_yuv_frame == NULL)
		{
			LOGE("yuv_frame malloc failed!\r\n");
			ret = BK_ERR_NO_MEM;
			return ret;
		}

		curr_yuv_frame->width = config->device->info.resolution.width;
		curr_yuv_frame->height = config->device->info.resolution.height;
		curr_yuv_frame->type = dvp_camera_config->device->type;
		curr_yuv_frame->fmt = current_sensor->fmt;
		curr_yuv_frame->length = size;

		if (encode_yuv_config == NULL)
		{
			encode_yuv_config = (encode_yuv_config_t *)os_malloc(sizeof(encode_yuv_config_t));
			if (encode_yuv_config == NULL)
			{
				LOGE("encode_yuv_config malloc error! \r\n");
				ret = BK_ERR_NO_MEM;
				return ret;
			}
		}

		encode_yuv_config->yuv_em_addr = bk_yuv_buf_get_em_base_addr();
		LOGI("yuv buffer base addr:%08x\r\n", encode_yuv_config->yuv_em_addr);
		encode_yuv_config->dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
		encode_yuv_config->yuv_pingpong_length = config->device->info.resolution.width * 8 * 2;
		encode_yuv_config->yuv_data_offset = 0;
		LOGI("dma_collect_yuv id is %d \r\n", encode_yuv_config->dma_collect_yuv);

		encode_yuv_dma_cpy(curr_yuv_frame->frame,
						   (uint32_t *)encode_yuv_config->yuv_em_addr,
						   encode_yuv_config->yuv_pingpong_length,
						   encode_yuv_config->dma_collect_yuv);
	}
#endif

	return ret;
}

static bk_err_t dvp_camera_h264_mode(dvp_camera_config_t *config)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	int ret = BK_OK;

#if CONFIG_YUV_BUF
	ret = dvp_camera_yuv_buf_config_init(H264_MODE);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = bk_h264_init((config->device->info.resolution.width << 16) | config->device->info.resolution.height);
	if (ret != BK_OK)
	{
		return ret;
	}

	config->fb_init(FB_INDEX_H264);

	ret = dvp_camera_dma_config();
	if (ret != BK_OK)
	{
		LOGE("dma init failed\n");
		return ret;
	}

#ifdef CONFIG_H264_ADD_SELF_DEFINE_SEI
	os_memset(&dvp_camera_drv->sei[0], 0xFF, H264_SELF_DEFINE_SEI_SIZE);

	h264_encode_sei_init(&dvp_camera_drv->sei[0]);
#endif

	if (config->device->mode == H264_YUV_MODE)
	{
		uint32_t size = config->device->info.resolution.width * config->device->info.resolution.height * 2;
		config->fb_init(FB_INDEX_DISPLAY);
		curr_yuv_frame = config->fb_malloc(FB_INDEX_DISPLAY, size);

		if(curr_yuv_frame == NULL)
		{
			LOGE("yuv_frame malloc failed!\r\n");
			ret = BK_ERR_NO_MEM;
			return ret;
		}

		curr_yuv_frame->width = config->device->info.resolution.width;
		curr_yuv_frame->height = config->device->info.resolution.height;
		curr_yuv_frame->fmt = current_sensor->fmt;
		curr_yuv_frame->type = dvp_camera_config->device->type;
		curr_yuv_frame->length = size;

		if (encode_yuv_config == NULL)
		{
			encode_yuv_config = (encode_yuv_config_t *)os_malloc(sizeof(encode_yuv_config_t));
			if (encode_yuv_config == NULL)
			{
				LOGE("encode_lcd_config malloc error! \r\n");
				ret = BK_ERR_NO_MEM;
				return ret;
			}
		}

		encode_yuv_config->yuv_em_addr = bk_yuv_buf_get_em_base_addr();
		LOGI("yuv buffer base addr:%08x\r\n", encode_yuv_config->yuv_em_addr);
		encode_yuv_config->dma_collect_yuv = bk_dma_alloc(DMA_DEV_DTCM);
		encode_yuv_config->yuv_pingpong_length = config->device->info.resolution.width * 16 * 2;
		encode_yuv_config->yuv_data_offset = 0;
		LOGI("dma_collect_yuv id is %d \r\n", encode_yuv_config->dma_collect_yuv);

		encode_yuv_dma_cpy(curr_yuv_frame->frame,
						   (uint32_t *)encode_yuv_config->yuv_em_addr,
						   encode_yuv_config->yuv_pingpong_length,
						   encode_yuv_config->dma_collect_yuv);
	}
#endif

	return ret;
}

static void dvp_camera_register_isr_function(yuv_mode_t mode)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	switch (mode)
	{
		case GRAY_MODE:
		case YUV_MODE:
			#if CONFIG_YUV_BUF
			bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, dvp_camera_yuv_eof_handler, NULL);
			#else
			bk_jpeg_enc_register_isr(JPEG_EOY, dvp_camera_yuv_eof_handler, NULL);
			#endif
			break;

		case JPEG_YUV_MODE:
		case JPEG_MODE:
			bk_jpeg_enc_register_isr(JPEG_EOF, dvp_camera_jpeg_eof_handler, NULL);
			#if CONFIG_YUV_BUF
			bk_jpeg_enc_register_isr(JPEG_FRAME_ERR, dvp_camera_sensor_ppi_err_handler, NULL);
			#endif
			break;

		case H264_YUV_MODE:
		case H264_MODE:
			#if CONFIG_H264
			bk_h264_register_isr(H264_FINAL_OUT, dvp_camera_h264_eof_handler, NULL);
			#endif
			break;

		default:
			break;
	}

#if CONFIG_YUV_BUF

	if (mode == JPEG_YUV_MODE || mode == H264_YUV_MODE)
	{
		bk_yuv_buf_register_isr(YUV_BUF_SM0_WR, (yuv_buf_isr_t)yuv_sm0_line_done, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_SM1_WR, (yuv_buf_isr_t)yuv_sm1_line_done, NULL);
		bk_yuv_buf_register_isr(YUV_BUF_YUV_ARV, dvp_camera_yuv_eof_handler, NULL);
	}

	bk_yuv_buf_register_isr(YUV_BUF_VSYNC_NEGEDGE, dvp_camera_vsync_negedge_handler, NULL);

	bk_yuv_buf_register_isr(YUV_BUF_SEN_RESL, dvp_camera_sensor_ppi_err_handler, NULL);
	bk_yuv_buf_register_isr(YUV_BUF_FULL, dvp_camera_sensor_ppi_err_handler, NULL);
	bk_yuv_buf_register_isr(YUV_BUF_H264_ERR, dvp_camera_sensor_ppi_err_handler, NULL);
	bk_yuv_buf_register_isr(YUV_BUF_ENC_SLOW, dvp_camera_sensor_ppi_err_handler, NULL);
#else
	bk_jpeg_enc_register_isr(JPEG_VSYNC_NEGEDGE, dvp_camera_vsync_negedge_handler, NULL);
#endif
}

const dvp_sensor_config_t *bk_dvp_camera_enumerate(void)
{
	i2c_config_t i2c_config = {0};

	// step 1: power on video modules
	bk_video_power_on(CONFIG_CAMERA_CTRL_POWER_GPIO_ID, 1);
	
	// step 2: map gpio as MCLK, PCLK for i2c communicate with dvp
	bk_video_gpio_init(DVP_GPIO_ALL);

	// step 3: enable mclk for i2c communicate with dvp
	bk_video_dvp_mclk_enable(YUV_MODE);

	// step 4: init i2c
	i2c_config.baud_rate = I2C_BAUD_RATE_100KHZ;
	i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
	bk_i2c_init(CONFIG_DVP_CAMERA_I2C_ID, &i2c_config);

	// step 5: detect sensor
	current_sensor = bk_dvp_get_sensor_auto_detect();
	if (current_sensor == NULL) {
		LOGE("%s no dvp camera found\n", __func__);
		return NULL;
	}

	LOGI("auto detect success, dvp camera name:%s\r\n", current_sensor->name);
	return current_sensor;
}

bk_err_t bk_dvp_camera_driver_init(dvp_camera_config_t *config)
{
	int ret = BK_OK;

	LOGI("%s\r\n", __func__);

	if (dvp_state != MASTER_TURN_OFF)
	{
		LOGE("dvp init state error\r\n");
		ret = BK_FAIL;
		return ret;
	}

	dvp_state = MASTER_TURNING_ON;

	DVP_DIAG_DEBUG_INIT();

	if (dvp_camera_config == NULL)
	{
		dvp_camera_config = (dvp_camera_config_t *)os_malloc(sizeof(dvp_camera_config_t));
		if (dvp_camera_config == NULL)
		{
			LOGE("%s, dvp_camera_config malloc failed!\r\n", __func__);
			goto error;
		}

#if !CONFIG_YUV_BUF
		if (config->device->mode == JPEG_YUV_MODE)
			config->device->mode = JPEG_MODE;
#endif

		os_memcpy(dvp_camera_config, config, sizeof(dvp_camera_config_t));

		dvp_camera_config->device = (media_camera_device_t *)os_malloc(sizeof(media_camera_device_t));
		if (dvp_camera_config->device == NULL)
		{
			LOGE("%s, dvp_camera_config->device malloc failed!\r\n", __func__);
			goto error;
		}

		os_memcpy(dvp_camera_config->device, config->device, sizeof(media_camera_device_t));
	}

	if (dvp_camera_drv == NULL)
	{
		dvp_camera_drv = (dvp_driver_t *)os_malloc(sizeof(dvp_driver_t));
		if (dvp_camera_drv == NULL)
		{
			LOGE("%s, dvp_camera_drv malloc failed!\r\n", __func__);
			goto error;
		}

		os_memset(dvp_camera_drv, 0, sizeof(dvp_driver_t));

		ret = rtos_init_semaphore(&dvp_camera_drv->sem, 1);
		if (ret != BK_OK)
		{
			LOGE("%s, dvp_camera_drv->sem malloc failed!\r\n", __func__);
			goto error;
		}
	}

	LOGI("%s, %d, mode:%d\r\n", __func__, __LINE__, dvp_camera_config->device->mode);

	/*if need cache psram too solw, you need use dma copy fifo data to sram, than copy to psram*/
	if (dvp_camera_config->device->mode == JPEG_MODE || dvp_camera_config->device->mode == JPEG_YUV_MODE)
	{
		#if !CONFIG_YUV_BUF
		dvp_camera_drv->cached = true;
		#endif
	}

	if (dvp_camera_drv->cached)
	{
		if (dvp_camera_drv->buffer == NULL)
		{
			dvp_camera_drv->buffer = (uint8_t *)os_malloc(FRAME_BUFFER_CACHE * 2);
		}

		dvp_camera_drv->psram_dma = bk_dma_alloc(DMA_DEV_DTCM);
		if ((dvp_camera_drv->psram_dma < DMA_ID_0) || (dvp_camera_drv->psram_dma >= DMA_ID_MAX))
		{
			LOGE("malloc dvp_camera_drv->psram_dma fail \r\n");
			ret = BK_FAIL;
			goto error;
		}

		LOGI("dvp_camera_drv->psram_dma:%d \r\n", dvp_camera_drv->psram_dma);
	}

	// step 1: for camera sensor, init other device
	ret = dvp_camera_init(dvp_camera_config->device->mode);
	if (ret != BK_OK)
	{
		goto error;
	}

	// step 2: config video hardware modules, include dma
	switch (dvp_camera_config->device->mode)
	{
		case YUV_MODE:
		case GRAY_MODE:
			ret = dvp_camera_yuv_mode(dvp_camera_config);
			break;

		case JPEG_MODE:
		case JPEG_YUV_MODE:
			ret = dvp_camera_jpeg_mode(dvp_camera_config);
			break;

		case H264_MODE:
		case H264_YUV_MODE:
			ret = dvp_camera_h264_mode(dvp_camera_config);
			break;

		default:
			ret = BK_FAIL;
	}

	if (ret != BK_OK)
	{
		goto error;
	}

	// step 3: maybe need register isr_func
	dvp_camera_register_isr_function(dvp_camera_config->device->mode);

	// step 4: start hardware function in different mode
	bk_video_set_mclk(current_sensor->clk);
	bk_video_encode_start(dvp_camera_config->device->mode);

	// step 5: init dvp camera sensor register
	current_sensor->init();
	current_sensor->set_ppi((dvp_camera_config->device->info.resolution.width << 16) | dvp_camera_config->device->info.resolution.height);
	current_sensor->set_fps(dvp_camera_config->device->info.fps);

	media_debug->isr_jpeg = 0;
	media_debug->isr_h264 = 0;
	media_debug->psram_busy = 0;
	media_debug->jpeg_length = 0;
	media_debug->h264_length = 0;
	media_debug->jpeg_kbps = 0;
	media_debug->h264_kbps = 0;

	dvp_state = MASTER_TURN_ON;

	return ret;

error:

	dvp_camera_deinit();

	return ret;
}

bk_err_t bk_dvp_camera_driver_deinit(void)
{
	if (dvp_state == MASTER_TURN_OFF)
	{
		LOGI("%s, dvp have been closed!\r\n", __func__);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	dvp_state = MASTER_TURNING_OFF;
	GLOBAL_INT_RESTORE();

	if (kNoErr != rtos_get_semaphore(&dvp_camera_drv->sem, 500))
	{
		LOGI("Not wait yuv vsync negedge!\r\n");
	}

	dvp_camera_deinit();

	return BK_OK;
}

media_camera_device_t *bk_dvp_camera_get_device(void)
{
	if (dvp_state == MASTER_TURN_ON)
		return dvp_camera_config->device;
	else
		return NULL;
}

bk_err_t bk_dvp_camera_free_encode_mem(void)
{
#if (CONFIG_ENCODE_BUF_NOT_FREE)
	if (dvp_camera_encode)
	{
		if (dvp_state == MASTER_TURN_OFF)
		{
			os_free(dvp_camera_encode);
			dvp_camera_encode = NULL;
			return BK_OK;
		}
		else
		{
			LOGI("camera is working, can not free\r\n");
			return BK_FAIL;
		}
	}
	else
	{
		LOGI("camera not malloc encode mem, can not free\r\n");
		return BK_OK;
	}
#else
	LOGW("marco CONFIG_ENCODE_BUF_NOT_FREE not enable!\r\n");
	return BK_FAIL;
#endif
}

bk_err_t bk_dvp_camera_h264_regenerate_idr_frame(void)
{
	if (dvp_state != MASTER_TURN_ON)
	{
		LOGW("%s, %d dvp not open!\r\n", __func__, __LINE__);
		return BK_FAIL;
	}

	if (dvp_camera_config == NULL
			|| dvp_camera_drv == NULL
			|| (dvp_camera_config->device->mode != H264_MODE && dvp_camera_config->device->mode != H264_YUV_MODE)
	)
	{
		LOGW("%s, %d not support this encode mode!\r\n", __func__, __LINE__);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	dvp_camera_drv->regenerate_idr = true;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}
