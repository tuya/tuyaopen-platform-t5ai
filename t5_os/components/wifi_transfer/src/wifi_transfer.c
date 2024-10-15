// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/mem.h>
#include <components/log.h>
#include "bk_wifi.h"
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "wifi_transfer.h"

#include "media_app.h"

#define TAG "wifi_trs"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static uint8_t transfer_enable = 0;
const media_transfer_cb_t *media_transfer_callback = NULL;
static transfer_data_t *transfer_app_data = NULL;
static video_setup_t *transfer_app_config = NULL;

#if (CONFIG_MEDIA_APP)
beken_thread_t wifi_transfer_net_camera_task = NULL;
static beken_semaphore_t s_wifi_transfer_net_data_sem;
bool wifi_transfer_net_camera_task_running = false;

wifi_transfer_net_camera_buffer_t *wifi_transfer_net_camera_buf = NULL;
wifi_transfer_net_camera_param_t wifi_transfer_net_camera_param = {0};
wifi_transfer_net_camera_pool_t wifi_transfer_net_camera_pool;
#endif

void wifi_transfer_data_check_caller(const char *func_name, int line,uint8_t *data, uint32_t length)
{
	if (length >= 12)
	{
		LOGI("%s %d data length %d\n",func_name,line,length);
		LOGI("%02x %02x %02x %02x %02x %02x \r\n", data[0], data[1], data[2], data[3], data[4], data[5]);
		LOGI("%02x %02x %02x %02x %02x %02x\r\n", data[6], data[7], data[8], data[9], data[10], data[11]);
		LOGI("\r\n");
	}
}

static int send_frame_buffer_packet(uint8_t *data, uint32_t size, uint32_t retry_max)
{
	int ret = BK_FAIL;
	uint16_t retry_cnt = 0;

	//wifi_transfer_data_check(data,size);

	if (media_transfer_callback == NULL)
		return ret;

	if (media_transfer_callback->prepare)
	{
		media_transfer_callback->prepare(data, size);
	}

	//wifi_transfer_data_check(data,size);

	do
	{
		ret = media_transfer_callback->send(data, size, &retry_cnt);

		if (ret == size)
		{
			break;
		}
		rtos_delay_milliseconds(TRAN_RETRY_DELAY_TIME);
	}
	while (retry_max-- && transfer_enable);

	return ret == size ? BK_OK : BK_FAIL;
}

static void wifi_transfer_send_frame(frame_buffer_t *buffer)
{
	int ret = kNoErr;

	uint32_t i;
	uint32_t count = buffer->length / transfer_app_config->pkt_size;
	uint32_t tail = buffer->length % transfer_app_config->pkt_size;

	uint8_t *src_address = buffer->frame;

	LOGD("%s %d transfer_app_config->pkt_size %d\n", __func__,__LINE__,transfer_app_config->pkt_size);

#if CONFIG_MEDIA_DROP_STRATEGY_ENABLE
	// check whether this frame should be dropped for some reasones such as no enough buffer
	if (media_transfer_callback->drop_check && media_transfer_callback->drop_check(buffer,(count + (tail ? 1 : 0)),transfer_app_config->pkt_header_size))
	{
		return;
	}
#endif

	transfer_app_data->id = (buffer->sequence & 0xFF);
	transfer_app_data->eof = 0;
	transfer_app_data->cnt = 0;//package seq num
	transfer_app_data->size = count + (tail ? 1 : 0);//one frame package counts

	LOGD("seq: %u, length: %u, size: %u count %d\n", buffer->sequence, buffer->length, buffer->size,transfer_app_data->size);

	for (i = 0; i < count && transfer_enable; i++)
	{
		transfer_app_data->cnt = i + 1;

		if ((tail == 0) && (i == count - 1))
		{
			transfer_app_data->eof = 1;
		}

		os_memcpy_word((uint32_t *)transfer_app_data->data, (uint32_t *)(src_address + (transfer_app_config->pkt_size * i)),
						transfer_app_config->pkt_size);

		LOGD("seq: %d [%d %d %d]\n", buffer->sequence,transfer_app_data->id,transfer_app_data->eof,transfer_app_data->cnt);

		ret = send_frame_buffer_packet((uint8_t *)transfer_app_data, transfer_app_config->pkt_size + transfer_app_config->pkt_header_size,
										TRAN_MAX_RETRY_TIME);
		if (ret != BK_OK)
		{
			LOGD("send failed\n");
		}
	}

	if (tail)
	{
		transfer_app_data->eof = 1;
		transfer_app_data->cnt = count + 1;

		os_memcpy_word((uint32_t *)transfer_app_data->data, (uint32_t *)(src_address + (transfer_app_config->pkt_size * i)),
						(tail % 4) ? ((tail / 4 + 1) * 4) : tail);

		LOGD("seq: %d [%d %d %d]\n", buffer->sequence,transfer_app_data->id,transfer_app_data->eof,transfer_app_data->cnt);

		if (1)//(transfer_app_config->send_type ==	TVIDEO_SND_UDP)
			ret = send_frame_buffer_packet((uint8_t *)transfer_app_data, tail + transfer_app_config->pkt_header_size, TRAN_MAX_RETRY_TIME);
		else
			ret = send_frame_buffer_packet((uint8_t *)transfer_app_data, VIDEO_TCP_TRAN_LEN, TRAN_MAX_RETRY_TIME);

		if (ret != BK_OK)
		{
			LOGD("send failed\n");
		}
	}

	LOGD("length: %u, tail: %u, count: %u\n", buffer->length, tail, count);
}

void wifi_transfer_read_frame_callback(frame_buffer_t *frame)
{
	wifi_transfer_send_frame(frame);
	if (frame->sequence < 8)
	{
		LOGI("%s, %d\r\n", __func__, frame->sequence);
	}
}

static bk_err_t wifi_transfer_buffer_init(const media_transfer_cb_t *cb)
{
	if (cb == NULL)
	{
		return BK_FAIL;
	}

	if (transfer_app_config == NULL)
	{
		transfer_app_config = (video_setup_t *)os_malloc(sizeof(video_setup_t));
		if (transfer_app_config == NULL)
		{
			LOGE("% malloc failed\r\n", __func__);
			return BK_ERR_NO_MEM;
		}

		os_memset(transfer_app_config, 0, sizeof(video_setup_t));
		transfer_app_config->pkt_header_size = sizeof(transfer_data_t);
	}

	if (cb->get_tx_buf)
	{
		transfer_app_data = cb->get_tx_buf();
		transfer_app_config->pkt_size = cb->get_tx_size() - transfer_app_config->pkt_header_size;

		if (transfer_app_data == NULL
			|| transfer_app_config->pkt_size <= 0)
		{
			LOGE("%s transfer_data: %p, size: %d\n", __func__, transfer_app_data, transfer_app_config->pkt_size);
			return BK_FAIL;
		}
	}
	else
	{
		if (transfer_app_data == NULL)
		{
			transfer_app_data = os_malloc(1472);
		}

		transfer_app_config->pkt_size = 1472 - transfer_app_config->pkt_header_size;
	}

	LOGI("%s transfer_data: %p, size: %d\n", __func__, transfer_app_data, transfer_app_config->pkt_size);

	media_transfer_callback = cb;

	return BK_OK;
}

static void wifi_transfer_buffer_deinit(void)
{
	if (transfer_app_config)
	{
		os_free(transfer_app_config);
		transfer_app_config = NULL;
	}

#ifdef CONFIG_INTEGRATION_DOORBELL
	media_transfer_callback = NULL;
	transfer_app_data = NULL;
#else
	if (transfer_app_data)
	{
		os_free(transfer_app_data);
		transfer_app_data = NULL;
	}
#endif
}

bk_err_t bk_wifi_transfer_frame_open(const media_transfer_cb_t *cb)
{
	int ret = BK_FAIL;

	if (transfer_enable)
	{
		LOGE("%s, have been opened!\r\n", __func__);
		return ret;
	}

	bk_wifi_set_wifi_media_mode(true);

	bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_SD);

	ret = wifi_transfer_buffer_init(cb);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = media_app_register_read_frame_callback(cb->fmt, wifi_transfer_read_frame_callback);

	if (ret == BK_OK)
	{
		transfer_enable = 1;
	}

	return ret;
}

bk_err_t bk_wifi_transfer_frame_close(void)
{
	int ret = BK_FAIL;

	if (!transfer_enable)
	{
		LOGE("%s, have been close!\r\n", __func__);
		return ret;
	}

	transfer_enable = 0;

	ret = media_app_unregister_read_frame_callback();

	wifi_transfer_buffer_deinit();

	media_transfer_callback = NULL;

	bk_wifi_set_wifi_media_mode(false);

	bk_wifi_set_video_quality(WIFI_VIDEO_QUALITY_HD);

	return ret;
}

#if (CONFIG_MEDIA_APP)
uint32_t wifi_transfer_net_tcp_checkout_eof(uint8_t *data, uint32_t length)
{
	uint32_t i =0;
	for (i = 0; i < length - 1; i++)
	{
		if (data[i] == 0xFF && data[i + 1] == 0xD9)
		{
			break;
		}
	}

	return (i + 2);
}

static bk_err_t wifi_transfer_net_camera_free_memory(void)
{
	if (wifi_transfer_net_camera_buf)
	{
		if (wifi_transfer_net_camera_buf->dma_id != DMA_ID_MAX)
		{
			bk_dma_stop(wifi_transfer_net_camera_buf->dma_id);
			bk_dma_deinit(wifi_transfer_net_camera_buf->dma_id);
			bk_dma_free(DMA_DEV_JPEG, wifi_transfer_net_camera_buf->dma_id);
		}

		if (wifi_transfer_net_camera_buf->dma_psram != DMA_ID_MAX)
		{
			bk_dma_stop(wifi_transfer_net_camera_buf->dma_psram);
			bk_dma_deinit(wifi_transfer_net_camera_buf->dma_psram);
			bk_dma_free(DMA_DEV_DTCM, wifi_transfer_net_camera_buf->dma_psram);
		}

		if (wifi_transfer_net_camera_buf->frame)
		{
			media_app_frame_buffer_clear(wifi_transfer_net_camera_buf->frame);
			wifi_transfer_net_camera_buf->frame = NULL;
		}

		wifi_transfer_net_camera_buf->buf_ptr = NULL;
		os_free(wifi_transfer_net_camera_buf);
		wifi_transfer_net_camera_buf = NULL;
	}

	if (wifi_transfer_net_camera_pool.pool)
	{
		os_free(wifi_transfer_net_camera_pool.pool);
		wifi_transfer_net_camera_pool.pool = NULL;
	}

	return BK_OK;
}

static bk_err_t wifi_transfer_net_camera_init_pool(void)
{
	if (wifi_transfer_net_camera_pool.pool == NULL)
	{
		wifi_transfer_net_camera_pool.pool = os_malloc(WIFI_RECV_CAMERA_POOL_LEN);
		if (wifi_transfer_net_camera_pool.pool == NULL)
		{
			LOGE("tvideo_pool alloc failed\r\n");
			return kNoMemoryErr;
		}
	}

	os_memset(&wifi_transfer_net_camera_pool.pool[0], 0, WIFI_RECV_CAMERA_POOL_LEN);

	trans_list_init(&wifi_transfer_net_camera_pool.free);
	trans_list_init(&wifi_transfer_net_camera_pool.ready);

	for (uint8_t i = 0; i < (WIFI_RECV_CAMERA_POOL_LEN / WIFI_RECV_CAMERA_RXNODE_SIZE); i++) {
		wifi_transfer_net_camera_pool.elem[i].buf_start =
			(void *)&wifi_transfer_net_camera_pool.pool[i * WIFI_RECV_CAMERA_RXNODE_SIZE];
		wifi_transfer_net_camera_pool.elem[i].buf_len = 0;

		trans_list_push_back(&wifi_transfer_net_camera_pool.free,
			(struct trans_list_hdr *)&wifi_transfer_net_camera_pool.elem[i].hdr);
	}

	return BK_OK;
}

static void wifi_transfer_net_camera_process_packet(uint8_t *data, uint32_t length)
{
	if ((wifi_transfer_net_camera_buf->start_buf == BUF_STA_INIT || wifi_transfer_net_camera_buf->start_buf == BUF_STA_COPY) && wifi_transfer_net_camera_buf->frame)
	{
		video_header_t *hdr = (video_header_t *)data;
		uint32_t org_len;
		uint32_t fack_len = 0;
		GLOBAL_INT_DECLARATION();

		org_len = length - sizeof(video_header_t);
		data = data + sizeof(video_header_t);

		LOGD("id:%d eof %d pkt cnt %d pkt seq %d len %d org_len %d\r\n", hdr->id,hdr->is_eof,hdr->pkt_cnt,hdr->pkt_seq,length,org_len);

		//wifi_transfer_data_check(data,size);

		if ((hdr->id != wifi_transfer_net_camera_buf->frame->sequence) && (hdr->pkt_cnt == 1)) {
			// start of frame;
			GLOBAL_INT_DISABLE();
			wifi_transfer_net_camera_buf->frame->sequence = hdr->id;
			wifi_transfer_net_camera_buf->frame->length = 0;
			wifi_transfer_net_camera_buf->frame_pkt_cnt = 0;
			wifi_transfer_net_camera_buf->buf_ptr = wifi_transfer_net_camera_buf->frame->frame;
			wifi_transfer_net_camera_buf->start_buf = BUF_STA_COPY;
			GLOBAL_INT_RESTORE();
			LOGD("sof:%d\r\n", wifi_transfer_net_camera_buf->frame->sequence);
		}
		else
		{
			if (wifi_transfer_net_camera_buf->start_buf == BUF_STA_INIT)
				wifi_transfer_net_camera_buf->start_buf = BUF_STA_COPY;
		}

	   /* LOGI("hdr-id:%d-%d, frame_packet_cnt:%d-%d, state:%d\r\n", hdr->id, wifi_recv_camera_buf->frame->sequence,
			(wifi_recv_camera_buf->frame_pkt_cnt + 1), hdr->pkt_cnt, wifi_recv_camera_buf->start_buf); */

		if ((hdr->id == wifi_transfer_net_camera_buf->frame->sequence)
			&& ((wifi_transfer_net_camera_buf->frame_pkt_cnt + 1) == hdr->pkt_cnt)
			&& (wifi_transfer_net_camera_buf->start_buf == BUF_STA_COPY))
		{
			if (wifi_transfer_net_camera_param.send_type == TVIDEO_SND_TCP && hdr->is_eof == 1)
			{
				org_len = wifi_transfer_net_tcp_checkout_eof(data, org_len);

				if (org_len & 0x3)
				{
					fack_len = ((org_len >> 2) + 1) << 2;
				}
			}

			if (fack_len == 0)
			{
				if (org_len & 0x3)
					fack_len = ((org_len >> 2) + 1) << 2;
			}

			if (wifi_transfer_net_camera_buf->dma_psram != DMA_ID_MAX)
			{
				dma_memcpy_by_chnl(wifi_transfer_net_camera_buf->buf_ptr, data, fack_len ? fack_len : org_len, wifi_transfer_net_camera_buf->dma_id);
			}
			else
			{
				os_memcpy(wifi_transfer_net_camera_buf->buf_ptr, data, fack_len ? fack_len : org_len);
			}

			GLOBAL_INT_DISABLE();
			wifi_transfer_net_camera_buf->frame->length += org_len;
			wifi_transfer_net_camera_buf->buf_ptr += org_len;
			wifi_transfer_net_camera_buf->frame_pkt_cnt += 1;
			GLOBAL_INT_RESTORE();

			if (hdr->is_eof == 1)
			{
				media_app_frame_buffer_push(wifi_transfer_net_camera_buf->frame);
				wifi_transfer_net_camera_buf->frame = media_app_frame_buffer_jpeg_malloc();
				if (wifi_transfer_net_camera_buf->frame == NULL)
				{
					LOGE("frame buffer malloc failed\r\n");
					return;
				}

				wifi_transfer_net_camera_buf->frame->width = ppi_to_pixel_x(wifi_transfer_net_camera_param.ppi);
				wifi_transfer_net_camera_buf->frame->height = ppi_to_pixel_y(wifi_transfer_net_camera_param.ppi);


				wifi_transfer_net_camera_buf->frame->fmt = wifi_transfer_net_camera_param.fmt; //all set uvc_jpeg, because jpeg need jepg decode
				wifi_transfer_net_camera_buf->buf_ptr = wifi_transfer_net_camera_buf->frame->frame;
				wifi_transfer_net_camera_buf->frame->length = 0;
			}
		}

	}
}

static void wifi_transfer_net_camera_task_entry(beken_thread_arg_t data)
{
	wifi_transfer_net_camera_elem_t *elem = NULL;
	bk_err_t err = 0;

	wifi_transfer_net_camera_buf->start_buf = BUF_STA_INIT;
	wifi_transfer_net_camera_task_running = true;

	while (wifi_transfer_net_camera_task_running)
	{
		err = rtos_get_semaphore(&s_wifi_transfer_net_data_sem, 1000);

		if(!wifi_transfer_net_camera_task_running)
		{
			break;
		}

		if(err != 0)
		{
			LOGD("%s get sem timeout\n", __func__);
			continue;
		}

		while((elem = (wifi_transfer_net_camera_elem_t *)trans_list_pick(&wifi_transfer_net_camera_pool.ready)) != NULL)
		{
			wifi_transfer_net_camera_process_packet(elem->buf_start, elem->buf_len);

			trans_list_pop_front(&wifi_transfer_net_camera_pool.ready);
			trans_list_push_back(&wifi_transfer_net_camera_pool.free, (struct trans_list_hdr *)&elem->hdr);
		}
	};

	rtos_deinit_semaphore(&s_wifi_transfer_net_data_sem);
	wifi_transfer_net_camera_task = NULL;
	rtos_delete_thread(NULL);

	wifi_transfer_net_camera_free_memory();
}

bk_err_t wifi_transfer_net_camera_open(media_camera_device_t *device)
{
	int ret = BK_OK;
	//step 1: init lcd, should do it after calling this api
	if (wifi_transfer_net_camera_buf == NULL)
	{
		wifi_transfer_net_camera_buf = (wifi_transfer_net_camera_buffer_t *)os_malloc(sizeof(wifi_transfer_net_camera_buffer_t));
		if (wifi_transfer_net_camera_buf == NULL)
		{
			LOGE("malloc net_camera_buf failed\r\n");
			goto error;
		}
	}

	os_memset(wifi_transfer_net_camera_buf, 0, sizeof(wifi_transfer_net_camera_buffer_t));

	wifi_transfer_net_camera_buf->dma_id = bk_dma_alloc(DMA_DEV_JPEG);
	if ((wifi_transfer_net_camera_buf->dma_id < DMA_ID_0) || (wifi_transfer_net_camera_buf->dma_id >= DMA_ID_MAX))
	{
		LOGE("malloc net_camera_buf->dma_id fail \r\n");
		wifi_transfer_net_camera_buf->dma_id = DMA_ID_MAX;
	}

	wifi_transfer_net_camera_buf->dma_psram = bk_dma_alloc(DMA_DEV_DTCM);
	if ((wifi_transfer_net_camera_buf->dma_psram < DMA_ID_0) || (wifi_transfer_net_camera_buf->dma_psram >= DMA_ID_MAX))
	{
		LOGE("malloc net_camera_buf->dma_id fail \r\n");
		wifi_transfer_net_camera_buf->dma_psram = DMA_ID_MAX;
	}

	LOGI("net_camera_buf->dma_id:%d-%d\r\n", wifi_transfer_net_camera_buf->dma_id, wifi_transfer_net_camera_buf->dma_psram);

	media_app_frame_buffer_init(FB_INDEX_JPEG);

	wifi_transfer_net_camera_buf->frame = media_app_frame_buffer_jpeg_malloc();
	if (wifi_transfer_net_camera_buf->frame == NULL)
	{
		goto error;
	}

	wifi_transfer_net_camera_buf->frame->fmt = device->fmt;

	wifi_transfer_net_camera_buf->buf_ptr = wifi_transfer_net_camera_buf->frame->frame;
	wifi_transfer_net_camera_buf->frame_pkt_cnt = 0;
	wifi_transfer_net_camera_buf->frame->width = device->info.resolution.width;
	wifi_transfer_net_camera_buf->frame->height = device->info.resolution.height;
	wifi_transfer_net_camera_buf->frame->sequence = 0;
	wifi_transfer_net_camera_buf->start_buf = BUF_STA_INIT;

	wifi_transfer_net_camera_param.ppi = (device->info.resolution.width << 16) | device->info.resolution.height;
	wifi_transfer_net_camera_param.fmt = wifi_transfer_net_camera_buf->frame->fmt;
	wifi_transfer_net_camera_param.send_type = TVIDEO_SND_TCP;

	ret = wifi_transfer_net_camera_init_pool();
	if (ret != BK_OK)
	{
		goto error;
	}

	ret = rtos_init_semaphore(&s_wifi_transfer_net_data_sem, 1);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&wifi_transfer_net_camera_task,
								6,
								"net_camera_task",
								(beken_thread_function_t)wifi_transfer_net_camera_task_entry,
								4 * 1024,
								NULL);

	if (BK_OK != ret)
	{
		LOGE("%s transfer_task init failed\n", __func__);
		goto error;
	}

	return BK_OK;

error:

	LOGE("%s failed\n", __func__);

	wifi_transfer_net_camera_free_memory();

	return BK_FAIL;
}

bk_err_t wifi_transfer_net_camera_close(void)
{
	if (wifi_transfer_net_camera_buf->start_buf == BUF_STA_COPY)
	{
		wifi_transfer_net_camera_buf->start_buf = BUF_STA_DEINIT;
	}

	wifi_transfer_net_camera_task_running = false;

	rtos_delay_milliseconds(100);

	wifi_transfer_net_camera_free_memory();

	LOGI("%s complete\r\n", __func__);

	return BK_OK;
}

uint32_t wifi_transfer_net_send_data(uint8_t *data, uint32_t length, video_send_type_t type)
{
	wifi_transfer_net_camera_elem_t *elem = NULL;

	if (wifi_transfer_net_camera_param.send_type != type)
		wifi_transfer_net_camera_param.send_type = type;
	if (length <= 4)
	{
		return length;
	}

	elem = (wifi_transfer_net_camera_elem_t *)trans_list_pick(&wifi_transfer_net_camera_pool.free);
	
	if (elem)
	{
		if (wifi_transfer_net_camera_buf->dma_id != DMA_ID_MAX)
		{
			dma_memcpy_by_chnl(elem->buf_start, data, length, wifi_transfer_net_camera_buf->dma_id);
		}
		else
		{
			os_memcpy(elem->buf_start, data, length);
		}

		elem->buf_len = length;

		trans_list_pop_front(&wifi_transfer_net_camera_pool.free);
		trans_list_push_back(&wifi_transfer_net_camera_pool.ready, (struct trans_list_hdr *)&elem->hdr);
		rtos_set_semaphore(&s_wifi_transfer_net_data_sem);
	}
	else
	{
		LOGI("list all busy\r\n");
	}

	return length;
}

#endif

