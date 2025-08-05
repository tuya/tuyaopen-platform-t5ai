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
#include <os/mem.h>
#include <driver/tp.h>
#include <driver/tp_types.h>
#include "tp_sensor_devices.h"


#define TAG "hy4633"
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


// external statement.
extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);


// macro define
#define HY4633_WRITE_ADDRESS     (0x70)
#define HY4633_READ_ADDRESS      (0x71)
#define HY4633_PRODUCT_ID_CODE   (0x33)

#define HY4633_ADDR_LEN          (1)
#define HY4633_REG_LEN           (1)
#define HY4633_MAX_TOUCH_NUM     (11)
#define HY4633_POINT_INFO_NUM    (TP_SUPPORT_MAX_NUM)
#define HY4633_POINT_INFO_SIZE   (6)
#define HY4633_POINT_INFO_TOTAL_SIZE  (HY4633_POINT_INFO_NUM * HY4633_POINT_INFO_SIZE)

#define HY4633_TP_RUN_MODE_REG   (0x00)
#define HY4633_STATUS            (0x02)
#define HY4633_POINT1_REG        (0x03)
#define HY4633_POINT2_REG        (0x09)
#define HY4633_POINT3_REG        (0x0F)
#define HY4633_POINT4_REG        (0x15)
#define HY4633_POINT5_REG        (0x1B)
#define HY4633_POINT6_REG        (0x21)
#define HY4633_POINT7_REG        (0x27)
#define HY4633_POINT8_REG        (0x2D)
#define HY4633_POINT9_REG        (0x33)
#define HY4633_POINT10_REG       (0x39)

#define HY4633_FW_VERSION_REG    (0xA6)
#define HY4633_LIB_VERSION_REG   (0xA7)
#define HY4633_TP_ID_REG         (0xA8)
#define HY4633_TP_CHIP_ID_REG    (0xA9)

#define HY4633_XY_CORDINATE_ROTATE_EN (0)

#define HY4633_REGS_DEBUG_EN (0)

#define SENSOR_I2C_READ(reg, buff, len)  cb->read_uint8((HY4633_WRITE_ADDRESS >> 1), reg, buff, len)
#define SENSOR_I2C_WRITE(reg, buff, len)  cb->write_uint8((HY4633_WRITE_ADDRESS >> 1), reg, buff, len)


bool hy4633_detect(const tp_i2c_callback_t *cb)
{
	if (NULL == cb)
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return false;
	}

	uint8_t product_id = 0;

	if (BK_OK != SENSOR_I2C_READ(HY4633_TP_CHIP_ID_REG, (uint8_t *)(&product_id), sizeof(product_id)))
	{
		LOGE("%s, read product id reg fail!\r\n", __func__);
		return false;
	}

	LOGD("%s, product id: 0X%02X\r\n", __func__, product_id);

	if (HY4633_PRODUCT_ID_CODE == product_id)
	{
		LOGD("%s success\n", __func__);
		return true;
	}

	return false;
}

int hy4633_init(const tp_i2c_callback_t *cb, tp_sensor_user_config_t *config)
{
	if ( (NULL == cb) || (NULL == config) )
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return BK_FAIL;
	}

#if 0
	int ret = BK_OK;
	uint8_t temp;

	temp = 0x00;
	if (BK_OK != SENSOR_I2C_WRITE(HY4633_TP_RUN_MODE_REG, (uint8_t *)&temp, sizeof(temp)))
	{
		LOGE("%s, write device mode config regs fail!\r\n", __func__);
		return BK_FAIL;
	}

	return ret;
#else
	return BK_OK;
#endif
}

int hy4633_read_status(const tp_i2c_callback_t *cb, uint8_t *status)
{
	if ( (NULL == cb) || (NULL == status) )
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return BK_FAIL;
	}

	if (BK_OK != SENSOR_I2C_READ(HY4633_STATUS, (uint8_t *)(status), 1))
	{
		LOGE("%s, read status reg fail!\r\n", __func__);
		return BK_FAIL;
	}

	LOGV("%s, status=0x%02X\r\n", __func__, *status);

	return BK_OK;
}

// hy4633 get tp info.
void hy4633_read_point(uint8_t *input_buff, void *buf, uint8_t num)
{
	uint8_t touch_num = num;
	uint8_t *read_buf = input_buff;
	tp_data_t *read_data = (tp_data_t *)buf;
	uint8_t read_index;
	uint8_t event_flag;
	uint8_t read_id;
	uint16_t input_x;
	uint16_t input_y;
	uint16_t input_w;
	uint8_t off_set;
	
	for (read_index = 0; read_index < touch_num; read_index++)
	{
		off_set = read_index * HY4633_POINT_INFO_SIZE;
		event_flag = (read_buf[off_set] >> 6) & 0x03;

		if (event_flag != 0x03)
		{
			read_id = (read_buf[off_set + 2] >> 4) & 0x0F;
			if (read_id >= HY4633_POINT_INFO_NUM)
			{
				LOGE("%s, touch ID %d is out range!\r\n", __func__, read_id);
				return;
			}
			
			input_x = ((read_buf[off_set] & 0x0F) << 8) | read_buf[off_set + 1];			/* x */
			input_y = ((read_buf[off_set + 2] & 0x0F) << 8) | (read_buf[off_set + 3]);		/* y */
			input_w = 0; 				                                                    /* weight | area */

			if (event_flag == 0x00)
			{
				read_data[read_id].event = TP_EVENT_TYPE_DOWN;
			}
			else if (event_flag == 0x01)
			{
				read_data[read_id].event = TP_EVENT_TYPE_UP;
			}
			else if (event_flag == 0x02)
			{
				read_data[read_id].event = TP_EVENT_TYPE_MOVE;
			}

			#if (HY4633_XY_CORDINATE_ROTATE_EN > 0)
				// special rotate process start
				int16_t sw = ppi_to_pixel_x(tp_sensor_hy4633.def_ppi); // tp screen width.
				int16_t sh = ppi_to_pixel_y(tp_sensor_hy4633.def_ppi); // tp screen high.

				int16_t temp = input_x;
				input_x = input_y;
				input_y = temp;
				LOGV("%s, [%d, %d]\r\n", __func__, input_x, input_y);

				#if 1
					// lcd rotates 90 degrees counterclockwise(adapt to lvgl).
					temp = input_x;
					input_x = input_y;
					input_y = sh - (temp + 1);
				#else
					// lcd rotates 270 degrees counterclockwise(adapt to lvgl).
					temp = input_y;
					input_y = input_x;
					input_x = sw - (temp + 1);
				#endif
				// special rotate process end
			#endif

			read_data[read_id].timestamp = rtos_get_time();
			read_data[read_id].width = input_w;
			read_data[read_id].x_coordinate = input_x;
			read_data[read_id].y_coordinate = input_y;
			read_data[read_id].track_id = read_id;
		}
	}
}

static uint8_t read_buff[3 + HY4633_POINT_INFO_TOTAL_SIZE];
int hy4633_read_tp_info(const tp_i2c_callback_t *cb, uint8_t max_num, uint8_t *buff)
{
	if ( (NULL == cb) || (NULL == buff) )
	{
		LOGE("%s, pointer is null!\r\n", __func__);
		return BK_FAIL;
	}

	if ( (0 == max_num) || (max_num > HY4633_POINT_INFO_NUM) )
	{
		LOGE("%s, max_num %d is out range!\r\n", __func__, max_num);
		return BK_FAIL;
	}

	int ret = BK_OK;
	uint8_t temp_status = 0;
	uint8_t read_num = 0;

	os_memset(read_buff, 0x00, sizeof(read_buff));
	if (BK_OK != SENSOR_I2C_READ(HY4633_TP_RUN_MODE_REG, (uint8_t *)read_buff, sizeof(read_buff)))
	{
		LOGE("%s, read tp info fail!\r\n", __func__);
		ret = BK_FAIL;
		goto exit_;
	}

	temp_status = read_buff[2];

	// original registers datas.
	LOGV("%s, status=0x%02X, pointer num is %d!\r\n", __func__, temp_status, temp_status&0x0F);
	#if (HY4633_REGS_DEBUG_EN > 0)
		bk_mem_dump_ex("tp_hy4633", (unsigned char *)(read_buff), sizeof(read_buff));
	#endif

	read_num = HY4633_POINT_INFO_NUM;
	
	LOGV("%s, read num is %d!\r\n", __func__, read_num);
	hy4633_read_point(read_buff+3, buff, read_num);

exit_:

	return ret;
}

const tp_sensor_config_t tp_sensor_hy4633 =
{
	.name = "hy4633",
	// default config
	.def_ppi = PPI_480X854,
	.def_int_type = TP_INT_TYPE_FALLING_EDGE,
	.def_refresh_rate = 10,
	.def_tp_num = 11,
	// capability config
	.id = TP_ID_HY4633,
	.address = (HY4633_WRITE_ADDRESS >> 1),
	.detect = hy4633_detect,
	.init = hy4633_init,
	.read_tp_info = hy4633_read_tp_info,
};
