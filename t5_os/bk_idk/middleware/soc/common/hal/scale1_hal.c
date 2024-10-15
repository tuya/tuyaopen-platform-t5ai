// Copyright 2021-2022 Beken
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

#include <common/bk_include.h>
#include "scale1_ll_macro_def.h"
#include "scale1_ll_macro_def.h"
#include <driver/hw_scale_types.h>
#include "bk_misc.h"
#include "scale1_hal.h"
#include "bk_misc.h"
#include <driver/media_types.h>


#define TAG "scale1"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
void scale1_hal_reset(void)
{
	scale1_ll_set_0x02_soft_reset(1);
	delay_us(10);
	scale1_ll_set_0x02_soft_reset(1);
	delay_us(10);
}

#if 0
bk_err_t scale1_hal_set_firstaddr(uint32_t addr)
{
	scale1_ll_set_0x04_first_addr(addr);
	
	return BK_OK;
}

bk_err_t scale1_hal_set_dest_addr(uint32_t addr)
{
	scale1_ll_set_0x0c_dest_addr(addr)
	return BK_OK;
}
#endif

//h264 mode: 16line, others:8line
bk_err_t  scale1_hal_set_pingpong_line_mode(scale_mode_t format)
{
	if (format == DVP_H264_SCALE)
		scale1_ll_set_0x05_line_index(1);
	else
		scale1_ll_set_0x05_line_index(0);
	return BK_OK;
}

bk_err_t scale1_hal_clear_int_status(void)
{
	scale1_ll_set_0x0f_clr_int_stat(1);

	return BK_OK;
}

bool scale1_hal_int_status_is_set(void)
{
	return scale1_ll_get_0x11_int_stat();
}

//just support 
bk_err_t scale1_hal_data_format(pixel_format_t fmt)
{
	switch (fmt)
	{
		case PIXEL_FMT_RGB565:
			scale1_ll_set_0x09_format(0);
			break;
		case PIXEL_FMT_YUYV:
			scale1_ll_set_0x09_format(1);
			break;
		case PIXEL_FMT_RGB888:
			scale1_ll_set_0x09_format(2);
			break;
		default:
			break;
	}
	return BK_OK;
}


bk_err_t scale1_hal_set_pic_width(uint16_t src_width, uint16_t dst_width)
{
	scale1_ll_set_0x0b_pic_width(src_width);
	scale1_ll_set_0x0d_out_pic_width(dst_width);
	return BK_OK;
}


bk_err_t scale1_hal_set_row_coef_loc_params(uint16_t dst_width, uint16_t *params)
{
	int i = 0;
	for ( i = 0; i < dst_width; i++)
	{
		*((volatile unsigned long *)(SCALE1_LL_REG_BASE + (0x20 + i)*4)) = params[i];
	}
	
	return BK_OK;
}

