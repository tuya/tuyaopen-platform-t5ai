#include <common/bk_include.h>
#include "bk_arm_arch.h"

#include <os/os.h>
#include "bk_uart.h"
#include <common/bk_kernel_err.h>
#include "video_transfer_common.h"

#include <components/video_transfer.h>

#include "bk_drv_model.h"
#include <os/mem.h>

#include <components/spidma.h>
#include <driver/timer.h>
#include <driver/dma.h>
#include <driver/uvc_camera.h>
#include <driver/jpeg_enc.h>
#include "media_app.h"

#if CONFIG_GENERAL_DMA
#include "bk_general_dma.h"
#endif

#include "camera_driver.h"

#define JPEG_EOF_CHECK              1
#define TVIDEO_DEBUG                1

#define TAG "video_tra"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define TVIDEO_TIMER_CHANNEL        TIMER_ID1
#define TVIDEO_TIMER_VALUE          2000// 1 second
#define TV_QITEM_COUNT              (120)
#define RTP_FU_HEAD_LEN				2

typedef struct rtp_fu_header_t {
	/* fu identifier */
	uint8_t id_type:		5;
	uint8_t id_ref:			2;
	uint8_t id_fbit:		1;

	/* fu header */
	uint8_t nal_type:		5;
	uint8_t f_bit:			1;
	uint8_t end_bit:		1;
	uint8_t start_bit:		1;
} rtp_fu_header_t;


video_config_t tvideo_st;
video_pool_t tvideo_pool;
video_setup_t video_transfer_setup_bak = {0};

beken_thread_t  tvideo_thread_hdl = NULL;
beken_queue_t tvideo_msg_que = NULL;
static uint8_t  g_dma_id = 0;
static uint32_t g_frame_total_num = 0;
static uint32_t g_lost_frame_id = 0;
//static uint8_t g_packet_count = 0;
static uint8_t s_packet_count = 0;
static bool tvideo_open = false;
static bool g_lost_flag = false;

bk_err_t tvideo_send_msg(uint8_t type, uint32_t data)
{
	bk_err_t ret;
	video_msg_t msg;

	if (tvideo_msg_que) {
		msg.type = type;
		msg.data = data;

		ret = rtos_push_to_queue(&tvideo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("tvideo_intfer_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t tvideo_pool_init(void *data)
{
	uint32_t i = 0;
	bk_err_t ret = kNoErr;
	video_setup_t *setup = (video_setup_t *)((int)data);

	if (tvideo_pool.pool == NULL) {
		tvideo_pool.pool = os_malloc(sizeof(uint8_t) * TVIDEO_POOL_LEN);
		if (tvideo_pool.pool == NULL) {
			LOGE("tvideo_pool alloc failed\r\n");
			ret = kNoMemoryErr;
			return ret;
		}
	}

	os_memset(&tvideo_pool.pool[0], 0, sizeof(uint8_t)*TVIDEO_POOL_LEN);

	co_list_init(&tvideo_pool.free);
	co_list_init(&tvideo_pool.ready);
#if TVIDEO_DROP_DATA_NONODE
	co_list_init(&tvideo_pool.receiving);
	tvideo_pool.drop_pkt_flag = 0;
#endif

	for (i = 0; i < (TVIDEO_POOL_LEN / TVIDEO_RXNODE_SIZE); i++) {
		tvideo_pool.elem[i].buf_start =
			(void *)&tvideo_pool.pool[i * TVIDEO_RXNODE_SIZE];
		tvideo_pool.elem[i].buf_len = 0;

		co_list_push_back(&tvideo_pool.free,
						  (struct co_list_hdr *)&tvideo_pool.elem[i].hdr);
	}

	LOGI("video transfer send type:%d, open type:%d\r\n",
			   setup->send_type, setup->open_type);

	tvideo_pool.open_type = setup->open_type;
	tvideo_pool.send_type = setup->send_type;
	tvideo_pool.send_func = setup->send_func;
	tvideo_pool.start_cb = setup->start_cb;
	tvideo_pool.end_cb = setup->end_cb;

#if(TVIDEO_USE_HDR)
	// sccb with camera interface on chip, or default
	if ((tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
		&& ((setup->pkt_header_size % 4) != 0)) {
		LOGW("pkt header-size should 4byte-aligned, but:%d\r\n",
					setup->pkt_header_size);
	}

	tvideo_pool.frame_id = 0;
	tvideo_pool.add_pkt_header = setup->add_pkt_header;
	tvideo_pool.pkt_header_size = setup->pkt_header_size;
#endif

	g_dma_id = bk_dma_alloc(DMA_DEV_DTCM);
	if ((g_dma_id < DMA_ID_0) || (g_dma_id >= DMA_ID_MAX))
	{
		LOGW("dma alloc failed!\r\n");
		g_dma_id = DMA_ID_MAX;
	}
	LOGI("video_transfer dma:%d\r\n", g_dma_id);

	return ret;
}

static void tvideo_timer_check_callback(timer_id_t timer_id)
{
	tvideo_send_msg(VIDEO_EOF_CHECK, 0);
}

static void tvideo_jpeg_eof_check_handler(void)
{
	//LOGI("frame_rate : %d\r\n", g_frame_total_num / 2);
#if JPEG_EOF_CHECK
	if (g_frame_total_num < 10)
	{
		if (tvideo_st.device->fmt == PIXEL_FMT_JPEG)
		{
			// jpeg eof error, reboot jpeg
			// step 1: stop timer
			bk_timer_stop(TVIDEO_TIMER_CHANNEL);
			media_app_rtsp_close();
			media_app_rtsp_open(&tvideo_st);
			g_frame_total_num = 0;

			// step 4: restart timer
			bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);
		}
	}
	else
	{
		//LOGI("jpg:%d\r\n", g_frame_total_num);
		g_frame_total_num = 0;
	}
#else
	//LOGI("jpg:%d\r\n", g_frame_total_num);
#endif
}

#define CHECK_HDR(ptr, i) ptr[i] == 0x1 && ptr[i + 1] == 0x0 \
						&& ptr[i + 2] == 0x0 && ptr[i + 3] == 0x0

static void memcpy_word_htonl(uint32_t *dst, uint32_t *src, uint32_t size)
{
	uint32_t i = 0;
	uint32_t tail = size % 4;
	size /= 4;

	for (i = 0; i < size; i++)
	{
		dst[i] = htonl(src[i]);
	}

	os_memcpy(dst, src, tail);
}

static int rtsp_h264_head_check(uint8_t *h264_data, uint8_t *send_head, uint32_t *data_len)
{
	int head_ofs[8] = {0};
	int head_cnt = 0;
	int head_dlen = 0;
	uint8_t *reverse_data = (uint8_t *)os_malloc(128);
	memcpy_word_htonl((uint32_t *)reverse_data, (uint32_t *)h264_data, 128);

	for (int i = 0; i < 128; i++)
	{
		if(CHECK_HDR(h264_data, i)) {
			head_ofs[head_cnt] = i;
			head_cnt ++;
		}
	}

	if (head_cnt > 7 || head_cnt == 0) {
		LOGE("too much head count! %d \r\n", head_cnt);
		return -1;
	}

	if (head_cnt == 1) {
		LOGD("only data frame! %d \r\n", head_cnt);
		return 0;
	}

	for (int i = 0; i < head_cnt - 1; i++)
	{
		int temp_len = head_ofs[i + 1] - head_ofs[i];
		if ((reverse_data[4] & 0x60) != 0) {
			os_memcpy(send_head + head_dlen, reverse_data + 4, temp_len - 4);
			head_dlen += (temp_len - 4);
		}
		reverse_data += temp_len;
		*data_len += temp_len;
	}
	os_free(reverse_data);
	return head_dlen;
}

static void tvideo_rx_handler(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len)
{
	video_elem_t *elem = NULL;
	// video_elem_t *head_elem = NULL;
	// uint32_t head_data_len = 0;
	//print_hex_dump(NULL, curptr, 8);
	s_packet_count++;

	if (is_eof) {
		g_frame_total_num++;
	}

	do {
		if (!newlen)
			break;

		if (tvideo_pool.frame_id == g_lost_frame_id && g_lost_flag)
		{
			goto out;
		}

#if TVIDEO_DROP_DATA_NONODE
		// drop pkt has happened, so drop it, until spidma timeout handler.
		if (tvideo_pool.drop_pkt_flag & TVIDEO_DROP_DATA_FLAG)
			break;
#endif

		elem = (video_elem_t *)co_list_pick(&tvideo_pool.free);
		if (elem)
		{
			g_lost_flag = false;

			if (newlen > tvideo_st.node_len)
				newlen = tvideo_st.node_len;

#if(TVIDEO_USE_HDR)
			// sccb with camera interface on chip, or default
			if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
			{
				video_packet_t param;

				param.ptk_ptr = (uint8_t *)elem->buf_start;
				param.ptklen = newlen;
				param.frame_id = tvideo_pool.frame_id;
				param.is_eof = is_eof;
				param.frame_len = s_packet_count;
				//print_hex_dump(NULL, curptr, 8);
				if (tvideo_pool.add_pkt_header)
					tvideo_pool.add_pkt_header(&param);

				//print_hex_dump(NULL, curptr, 8);
#if 0
				uint8_t head_pkt[128];
				head_data_len = 0;
				int head_pkt_len = rtsp_h264_head_check(curptr, head_pkt, &head_data_len);
				if(head_pkt_len > 0) {

					head_elem = (video_elem_t *)co_list_pick(&tvideo_pool.free);
					if (head_elem) {
						co_list_pop_front(&tvideo_pool.free);
						video_packet_t head_param;
						head_param.ptk_ptr = (uint8_t *)head_elem->buf_start;
						head_param.ptklen = head_pkt_len;
						head_param.frame_id = tvideo_pool.frame_id;
						head_param.is_eof = 1;
						head_param.frame_len = s_packet_count;

						curptr += head_data_len;
						newlen -= head_data_len;

						if (tvideo_pool.add_pkt_header)
							tvideo_pool.add_pkt_header(&head_param);

						os_memcpy(head_param.ptk_ptr + tvideo_pool.pkt_header_size, head_pkt, head_pkt_len);
						head_elem->buf_len = head_pkt_len + tvideo_pool.pkt_header_size;
						head_elem->frame_id = tvideo_pool.frame_id;
						co_list_push_back(&tvideo_pool.ready, (struct co_list_hdr *)&head_elem->hdr);
						tvideo_send_msg(VIDEO_SEND, 0);

					} else {
						LOGE("%s lost head\r\n", __func__);
					}
				}
#endif
				rtp_fu_header_t rtp_fu_header;
				rtp_fu_header.id_fbit = 0;
				rtp_fu_header.id_ref = 0;
				rtp_fu_header.id_type = 0x1C;
				rtp_fu_header.end_bit = is_eof;
				//print_hex_dump(NULL, curptr, 8);
				rtp_fu_header.start_bit = (*(uint32_t *)curptr == 0x01000000) ? 1 : 0;
				//print_hex_dump(NULL, curptr, 8);
				rtp_fu_header.f_bit = 0;
				rtp_fu_header.nal_type = 0;
				

				os_memcpy(param.ptk_ptr + tvideo_pool.pkt_header_size, &rtp_fu_header, sizeof(rtp_fu_header_t));
				//print_hex_dump(NULL, curptr, 8);

				// if (g_dma_id != DMA_ID_MAX)
				// {
				// 	dma_memcpy_by_chnl(param.ptk_ptr + tvideo_pool.pkt_header_size + RTP_FU_HEAD_LEN, curptr, newlen, g_dma_id);
				// }
				// else
				// {
				// 	os_memcpy(param.ptk_ptr + tvideo_pool.pkt_header_size + RTP_FU_HEAD_LEN, curptr, newlen);
				// }
				os_memcpy((uint32_t *)(param.ptk_ptr + tvideo_pool.pkt_header_size + RTP_FU_HEAD_LEN), curptr, newlen);
				//print_hex_dump(NULL, curptr, 8);

				if (tvideo_st.node_len > newlen)
				{
					//uint32_t left = tvideo_st.node_len - newlen;
					//os_memset((elem_tvhdr + 1 + newlen), 0, left);
				}

				elem->buf_len = newlen + tvideo_pool.pkt_header_size + RTP_FU_HEAD_LEN;
				elem->frame_id = tvideo_pool.frame_id;
				//print_hex_dump(NULL, curptr, 8);
			} else
#endif //#if (TVIDEO_USE_HDR)
			{
				// only copy data
				if (g_dma_id != DMA_ID_MAX) {
					dma_memcpy_by_chnl(elem->buf_start, curptr, newlen, g_dma_id);
				} else{
					os_memcpy(elem->buf_start, curptr, newlen);
				}

				if (tvideo_st.node_len > newlen) {
					//uint32_t left = tvideo_st.node_len - newlen;
					//os_memset(((uint8_t*)elem->buf_start + newlen), 0, left);
				}
				//elem->buf_len = tvideo_st.node_len;
				elem->buf_len = newlen;
			}

			co_list_pop_front(&tvideo_pool.free);
#if TVIDEO_DROP_DATA_NONODE
			co_list_push_back(&tvideo_pool.receiving, (struct co_list_hdr *)&elem->hdr);
#else
			co_list_push_back(&tvideo_pool.ready, (struct co_list_hdr *)&elem->hdr);
			//os_printf("push to ready \r\n");
#endif
			tvideo_send_msg(VIDEO_SEND, 0);
		}
		else
		{
#if TVIDEO_DROP_DATA_NONODE
			// not node for receive pkt, drop aready received, and also drop
			// the new come.
			uint32_t cnt_rdy = co_list_cnt(&tvideo_pool.receiving);

			tvideo_pool.drop_pkt_flag |= TVIDEO_DROP_DATA_FLAG;
			if (cnt_rdy)
				co_list_concat(&tvideo_pool.free, &tvideo_pool.receiving);
#else
			os_printf("@flost\r\n");
			g_lost_flag = true;
			g_lost_frame_id = tvideo_pool.frame_id;
#endif
		}
	} while (0);

out:

	if (is_eof)
	{
		s_packet_count = 0;
	}
}

static void tvideo_end_frame_handler(void)
{
#if TVIDEO_DROP_DATA_NONODE
	// reset drop flag, new pkt can receive
	tvideo_pool.drop_pkt_flag &= (~TVIDEO_DROP_DATA_FLAG);
	if (!co_list_is_empty(&tvideo_pool.receiving))
		co_list_concat(&tvideo_pool.ready, &tvideo_pool.receiving);
#endif

#if(TVIDEO_USE_HDR)
	if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
		tvideo_pool.frame_id++;
#endif

	tvideo_send_msg(VIDEO_SEND, 0);
}

static void tvideo_drop_frame_handler(uint8_t index)
{
	tvideo_send_msg(VIDEO_DROP, index);
}

static bk_err_t tvideo_config_desc(void)
{
	bk_err_t ret = kNoErr;
	uint32_t node_len = TVIDEO_RXNODE_SIZE_TCP;

	if (tvideo_pool.send_type == TVIDEO_SND_UDP) {
#if(TVIDEO_USE_HDR)
		if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
			node_len = TVIDEO_RXNODE_SIZE_UDP - tvideo_pool.pkt_header_size - 20;  // to do configure packet size
		else
#endif
			node_len = TVIDEO_RXNODE_SIZE_UDP;

	} else if (tvideo_pool.send_type == TVIDEO_SND_TCP)
		node_len = TVIDEO_RXNODE_SIZE_TCP - tvideo_pool.pkt_header_size - 4;  // to do configure packet size
	else if (tvideo_pool.send_type == TVIDEO_SND_INTF) {
#if(TVIDEO_USE_HDR)
		node_len = TVIDEO_RXNODE_SIZE_UDP - tvideo_pool.pkt_header_size;
#else
		node_len = TVIDEO_RXNODE_SIZE_UDP;
#endif
	} else if (tvideo_pool.send_type == TVIDEO_SND_BUFFER)
		node_len = TVIDEO_RXNODE_SIZE_TCP;
	else {
		LOGW("Err snd tpye in spidma\r\n");
		ret = kParamErr;
		return ret;
	}

	if (tvideo_st.rxbuf == NULL)
	{
		tvideo_st.rxbuf = os_malloc(sizeof(uint8_t) * TVIDEO_RXBUF_LEN);
		if (tvideo_st.rxbuf == NULL) {
			LOGW("malloc rxbuf failed!\r\n");
			ret = kNoMemoryErr;
			return ret;
		}
	}

	tvideo_st.rxbuf_len = node_len * 4;
	tvideo_st.node_len = node_len;
	tvideo_st.rx_read_len = 0;

	tvideo_st.device = video_transfer_setup_bak.device;
	tvideo_st.node_full_handler = tvideo_rx_handler;
	tvideo_st.data_end_handler = tvideo_end_frame_handler;

	return ret;
}

static void tvideo_poll_handler(void)
{
	uint32_t send_len;
	video_elem_t *elem = NULL;

	do {
		elem = (video_elem_t *)co_list_pick(&tvideo_pool.ready);
		if (elem) {
			if (tvideo_pool.send_func) {
				/*if (elem->frame_id == g_lost_frame_id) {

				}
				else*/
				{
					send_len = tvideo_pool.send_func(elem->buf_start, elem->buf_len);
					if (send_len != elem->buf_len) {
						//os_printf("sendlen :%d buflen: :%d \r\n",send_len, elem->buf_len);
						break;
					}
				}
			}

			co_list_pop_front(&tvideo_pool.ready);
			co_list_push_back(&tvideo_pool.free, (struct co_list_hdr *)&elem->hdr);
		}
	} while (elem);
}

static void tvideo_jpeg_drop_frame(uint32_t index)
{
	// to do
}


/*---------------------------------------------------------------------------*/
static void video_transfer_main(beken_thread_arg_t data)
{
	bk_err_t err;
	LOGI("=======video_transfer_main entry==========\r\n");

	err = tvideo_pool_init(data);
	if (err != kNoErr) {
		goto tvideo_exit;
	}

	err = tvideo_config_desc();
	if (err != kNoErr) {
		goto tvideo_exit;
	}

	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {

#if CONFIG_SPIDMA
		spidma_intfer_init(&tvideo_st);
#endif
		} else {
			if (tvideo_st.device->fmt == PIXEL_FMT_JPEG || tvideo_st.device->fmt == PIXEL_FMT_H264)
			{
				err = media_app_rtsp_open(&tvideo_st);
				if (err != kNoErr) {
					goto tvideo_exit;
				}
			}
			else
			{
#if CONFIG_USB_UVC
				//err = _bk_uvc_camera_init(&tvideo_st);
				if (err != kNoErr)
				{
					goto tvideo_exit;
				}
#endif
			}

			tvideo_open = true;
	}

	if (tvideo_pool.start_cb != NULL)
		tvideo_pool.start_cb();

	bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);

	while (1) {
		video_msg_t msg;
		err = rtos_pop_from_queue(&tvideo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == err) {
			switch (msg.type) {
			case VIDEO_SEND:
				tvideo_poll_handler();
				break;

			case VIDEO_EOF_CHECK:
				tvideo_jpeg_eof_check_handler();
				break;

			case VIDEO_EXIT:
				goto tvideo_exit;
				break;

			case VIDEO_DROP:
				tvideo_jpeg_drop_frame(msg.data);
				break;

			default:
				break;
			}
		}
	}

	if (tvideo_pool.end_cb != NULL)
		tvideo_pool.end_cb();

tvideo_exit:
	LOGI("video_transfer_main exit\r\n");

	bk_timer_stop(TVIDEO_TIMER_CHANNEL);

	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {
#if CONFIG_SPIDMA
		spidma_intfer_deinit();
#endif
	} else {
		if (tvideo_open)
		{
			if (tvideo_st.device->type == DVP_CAMERA)
			{
				err = media_app_rtsp_close();
			}
			else
			{
#if CONFIG_USB_UVC
				//err = _bk_uvc_camera_deinit();
#endif
			}

			tvideo_open = false;
		}
	}

	g_frame_total_num = 0;

	if (g_dma_id != DMA_ID_MAX)
	{
		bk_dma_stop(g_dma_id);

		bk_dma_deinit(g_dma_id);

		bk_dma_free(DMA_DEV_DTCM, g_dma_id);
	}

	if (tvideo_pool.pool) {
		os_free(tvideo_pool.pool);
		tvideo_pool.pool = NULL;
	}

	if (tvideo_st.rxbuf) {
		os_free(tvideo_st.rxbuf);
		tvideo_st.rxbuf = NULL;
	}

	os_memset(&tvideo_st, 0, sizeof(video_config_t));

	rtos_deinit_queue(&tvideo_msg_que);
	tvideo_msg_que = NULL;

	tvideo_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t bk_video_transfer_init(video_setup_t *setup_cfg)
{
	int ret;

	if ((!tvideo_thread_hdl) && (!tvideo_msg_que)) {

		os_memcpy(&video_transfer_setup_bak, setup_cfg, sizeof(video_setup_t));

		ret = rtos_init_queue(&tvideo_msg_que,
							  "tvideo_queue",
							  sizeof(video_msg_t),
							  TV_QITEM_COUNT);
		if (kNoErr != ret) {
			return kGeneralErr;
		}

		ret = rtos_create_thread(&tvideo_thread_hdl,
								 4,
								 "video_intf",
								 (beken_thread_function_t)video_transfer_main,
								 4 * 1024,
								 (beken_thread_arg_t)&video_transfer_setup_bak);
		if (ret != kNoErr) {
			rtos_deinit_queue(&tvideo_msg_que);
			tvideo_msg_que = NULL;
			tvideo_thread_hdl = NULL;
			return kGeneralErr;
		}

		return kNoErr;
	} else
		return kInProgressErr;
}

bk_err_t bk_video_transfer_deinit(void)
{
	LOGI("video_transfer_deinit\r\n");

	tvideo_send_msg(VIDEO_EXIT, 0);

	while (tvideo_thread_hdl)
		rtos_delay_milliseconds(10);

	return kNoErr;
}


bk_err_t bk_video_transfer_stop(void)
{
	int ret = kNoErr;

	// stop jpeg
	if (!tvideo_open)
	{
		LOGI("video_transfer_stop already!\r\n");
		return ret;
	}

	bk_timer_stop(TVIDEO_TIMER_CHANNEL);

	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {
#if CONFIG_SPIDMA
		spidma_intfer_deinit();
#endif
	} else {
		if (tvideo_st.device->fmt == PIXEL_FMT_JPEG)
		{
			ret = bk_dvp_camera_deinit();
		}
		else if (tvideo_st.device->fmt == PIXEL_FMT_H264)
		{
			ret = bk_dvp_camera_deinit();
		}
		else
		{
#if CONFIG_USB_UVC
			// ret = bk_uvc_camera_stop();
#endif
		}
	}

	tvideo_open = false;

	return ret;
}

bk_err_t bk_video_transfer_start(void)
{
	int ret = kNoErr;

	if (tvideo_open)
	{
		LOGI("video_transfer_start already!\r\n");
		return ret;
	}

	// start jpeg
	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA)
	{

#if CONFIG_SPIDMA
		spidma_intfer_init(&tvideo_st);
#endif
	}
	else
	{
		if (tvideo_st.device->type == DVP_CAMERA)
		{
			ret = media_app_rtsp_open(&tvideo_st);
		}
		else
		{
#if CONFIG_USB_UVC
			// ret = bk_uvc_camera_start();
#endif
		}
	}

	if (ret != kNoErr)
	{
		LOGI("video_transfer_start failed!\r\n");
		return ret;
	}

	bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);

	tvideo_open = true;

	return ret;
}
