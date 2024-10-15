#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/int.h>
#include <common/bk_err.h>
#ifdef CONFIG_RTT
#include <sys/socket.h>
#endif
#include "lwip/sockets.h"
#include "bk_uart.h"
#include <components/video_transfer.h>
#include <driver/dma.h>
#include <driver/audio_ring_buff.h>
#include "aud_intf.h"
#include "aud_intf_types.h"
#include "lcd_act.h"
#include "av_server_comm.h"
#include "av_server_udp_service.h"
#include "av_server_devices.h"
#include "media_app.h"
#include "wifi_transfer.h"

#define TAG "av_server-UDP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
	beken_thread_t thd;
	struct sockaddr_in img_remote;
	struct sockaddr_in aud_remote;
	int img_fd;
	int aud_fd;
	uint16_t running : 1;
	uint16_t img_status : 1;
	uint16_t aud_status : 1;
	db_channel_t *img_channel;
	db_channel_t *aud_channel;
} db_udp_service_t;

db_udp_service_t *db_udp_service = NULL;

camera_parameters_t camera_parameters = {UVC_DEVICE_ID,800,480,0,1,65535};
lcd_parameters_t lcd_parameters = {10,90,0};
audio_parameters_t audio_parameters = {1,0,1,1,8000,8000};


#if AUDIO_TRANSFER_ENABLE
static aud_intf_drv_setup_t aud_intf_drv_setup = DEFAULT_AUD_INTF_DRV_SETUP_CONFIG();
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;
static aud_intf_voc_setup_t aud_voc_setup = DEFAULT_AUD_INTF_VOC_SETUP_CONFIG();
#endif

#if 0//(!CONFIG_AV_DEMO_MODE_TCP)
beken_semaphore_t s_av_server_service_sem;
#endif

static struct sockaddr_in video_sender;
static struct sockaddr_in audio_sender;

int av_server_udp_img_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	if (!db_udp_service->img_status)
	{
		return -1;
	}

	return av_server_socket_sendto(&db_udp_service->img_fd, (struct sockaddr *)&video_sender, data, len, -sizeof(db_trans_head_t), retry_cnt);
}


int av_server_udp_img_send_prepare(uint8_t *data, uint32_t length)
{
	av_server_transmission_pack(db_udp_service->img_channel, data, length);

	return 0;
}

void *av_server_udp_img_get_tx_buf(void)
{
	if (db_udp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return NULL;
	}

	if (db_udp_service->img_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return NULL;
	}

	LOGI("%s, tbuf %p\n",__func__, db_udp_service->img_channel->tbuf);

	return db_udp_service->img_channel->tbuf + 1;
}

int av_server_udp_img_get_tx_size(void)
{
	if (db_udp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return 0;
	}

	if (db_udp_service->img_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return 0;
	}

	return db_udp_service->img_channel->tsize - sizeof(db_trans_head_t);
}


int av_server_udp_aud_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	if (!db_udp_service->aud_status)
	{
		return -1;
	}

	return av_server_socket_sendto(&db_udp_service->aud_fd, (struct sockaddr *)&audio_sender, data, len, -sizeof(db_trans_head_t), retry_cnt);
}

int av_server_udp_aud_send_prepare(uint8_t *data, uint32_t length)
{

	av_server_transmission_pack(db_udp_service->aud_channel, data, length);

	return 0;
}

void *av_server_udp_aud_get_tx_buf(void)
{
	if (db_udp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return NULL;
	}

	if (db_udp_service->aud_channel == NULL)
	{
		LOGE("%s, aud_channel null\n",__func__);
		return NULL;
	}

	LOGD("%s, tbuf %p\n",__func__, db_udp_service->aud_channel->tbuf);

	return db_udp_service->aud_channel->tbuf + 1;
}

int av_server_udp_aud_get_tx_size(void)
{
	if (db_udp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return 0;
	}

	if (db_udp_service->aud_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return 0;
	}

	return db_udp_service->aud_channel->tsize - sizeof(db_trans_head_t);
}

static media_transfer_cb_t av_server_udp_img_channel =
{
	.send = av_server_udp_img_send_packet,
	.prepare = av_server_udp_img_send_prepare,
	.get_tx_buf = av_server_udp_img_get_tx_buf,
	.get_tx_size = av_server_udp_img_get_tx_size,
	.fmt = PIXEL_FMT_JPEG
};

static const media_transfer_cb_t av_server_udp_aud_channel =
{
	.send = av_server_udp_aud_send_packet,
	.prepare = av_server_udp_aud_send_prepare,
	.get_tx_buf = av_server_udp_aud_get_tx_buf,
	.get_tx_size = av_server_udp_aud_get_tx_size,
	.fmt = PIXEL_FMT_UNKNOW
};

static inline void av_server_udp_voice_receiver(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
	LOGD("%s %d\n", __func__, length);
	av_server_audio_data_callback(data, length);
}

static inline void av_server_udp_video_receiver(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
	//wifi_transfer_data_check(data,length);
#if (CONFIG_MEDIA_APP)
	wifi_transfer_net_send_data(data, length, TVIDEO_SND_UDP);
#endif
}

static void av_server_udp_service_main(beken_thread_arg_t data)
{
	GLOBAL_INT_DECLARATION();
	int maxfd, ret = 0;
	int rcv_len = 0;
	socklen_t srvaddr_len = 0;
	fd_set watchfd;
	struct timeval timeout;
	u8 *rcv_buf = NULL;

	LOGI("%s, img: %d, aud: %d\n",__func__, AV_SERVER_UDP_IMG_PORT, AV_SERVER_UDP_AUD_PORT);
	(void)(data);

#if 0//(!CONFIG_AV_DEMO_MODE_TCP)
	ret = rtos_get_semaphore(&s_av_server_service_sem, BEKEN_WAIT_FOREVER);
#endif

	rcv_buf = (u8 *)os_malloc((AV_SERVER_NETWORK_MAX_SIZE + 1) * sizeof(u8));
	if (!rcv_buf)
	{
		LOGE("udp os_malloc failed\n");
		goto out;
	}


	// for data transfer
	db_udp_service->img_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (db_udp_service->img_fd == -1)
	{
		LOGE("socket failed\n");
		goto out;
	}

	db_udp_service->img_remote.sin_family = AF_INET;
	db_udp_service->img_remote.sin_port = htons(AV_SERVER_UDP_IMG_PORT);
	db_udp_service->img_remote.sin_addr.s_addr = inet_addr("0.0.0.0");

	video_sender.sin_family = AF_INET;
	video_sender.sin_port = htons(AV_SERVER_UDP_IMG_PORT);
	video_sender.sin_addr.s_addr = inet_addr("192.168.188.100");

	srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);

	if (bind(db_udp_service->img_fd, (struct sockaddr *)&db_udp_service->img_remote, srvaddr_len) == -1)
	{
		LOGE("bind failed\n");
		goto out;
	}


	db_udp_service->aud_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (db_udp_service->aud_fd == -1)
	{
		LOGE("vo socket failed\n");
		goto out;
	}

	db_udp_service->aud_remote.sin_family = AF_INET;
	db_udp_service->aud_remote.sin_port = htons(AV_SERVER_UDP_AUD_PORT);
	db_udp_service->aud_remote.sin_addr.s_addr =  inet_addr("0.0.0.0");

	audio_sender.sin_family = AF_INET;
	audio_sender.sin_port = htons(AV_SERVER_UDP_AUD_PORT);
	audio_sender.sin_addr.s_addr = inet_addr("192.168.188.100");

	srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);

	if (bind(db_udp_service->aud_fd, (struct sockaddr *)&db_udp_service->aud_remote, srvaddr_len) == -1)
	{
		LOGE("bind failed\n");
		goto out;
	}

	maxfd = (db_udp_service->img_fd > db_udp_service->aud_fd) ? db_udp_service->img_fd : db_udp_service->aud_fd;

	timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
	timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;


	GLOBAL_INT_DISABLE();
	db_udp_service->img_status = 1;
	db_udp_service->running = 1;
	db_udp_service->aud_status = 1;
	GLOBAL_INT_RESTORE();

	ret = av_server_camera_turn_on(&camera_parameters);

	if (ret != BK_OK)
	{
		LOGE("turn on video transfer failed\n");
		goto out;
	}

	ret = av_server_display_turn_on(lcd_parameters.id, lcd_parameters.rotate, lcd_parameters.fmt);

	if (ret != BK_OK)
	{
		LOGE("turn on display failed\n");
		goto out;
	}

	ret = av_server_audio_turn_on(&audio_parameters);

	if (ret != BK_OK)
	{
		LOGE("turn on audio failed\n");
		goto out;
	}

	while (db_udp_service->running)
	{
		FD_ZERO(&watchfd);
		FD_SET(db_udp_service->img_fd, &watchfd);
		FD_SET(db_udp_service->aud_fd, &watchfd);

		//DBD("wait for select\n");
		ret = select(maxfd + 1, &watchfd, NULL, NULL, &timeout);
		if (ret < 0)
		{
			LOGE("select ret:%d\n", ret);
			break;
		}
		else
		{
			// is img fd, data transfer
			if (FD_ISSET(db_udp_service->img_fd, &watchfd))
			{
				rcv_len = recvfrom(db_udp_service->img_fd, rcv_buf, AV_SERVER_NETWORK_MAX_SIZE, 0,
								   (struct sockaddr *)&video_sender, &srvaddr_len);

				if (rcv_len <= 0)
				{
					// close this socket
					LOGE("recv close fd:%d, error_code:%d\n", db_udp_service->img_fd, errno);
					break;
				}
				else
				{
					rcv_len = (rcv_len > AV_SERVER_NETWORK_MAX_SIZE) ? AV_SERVER_NETWORK_MAX_SIZE : rcv_len;
					rcv_buf[rcv_len] = 0;

					av_server_transmission_unpack(db_udp_service->img_channel, rcv_buf, rcv_len, av_server_udp_video_receiver);
				}
			}

			if (FD_ISSET(db_udp_service->aud_fd, &watchfd))
			{
				rcv_len = recvfrom(db_udp_service->aud_fd, rcv_buf, AV_SERVER_NETWORK_MAX_SIZE, 0,
								   (struct sockaddr *)&audio_sender, &srvaddr_len);

				if (rcv_len <= 0)
				{
					// close this socket
					LOGE("recv close fd:%d, error_code:%d\n", db_udp_service->aud_fd, errno);
					break;
				}
				else
				{
					rcv_len = (rcv_len > AV_SERVER_NETWORK_MAX_SIZE) ? AV_SERVER_NETWORK_MAX_SIZE : rcv_len;
					rcv_buf[rcv_len] = 0;

					av_server_transmission_unpack(db_udp_service->aud_channel, rcv_buf, rcv_len, av_server_udp_voice_receiver);

				}
			}
		}
	}

out:

	LOGE("%s exit %d\n",__func__, db_udp_service->running);

	av_server_udp_service_deinit();

	if (rcv_buf)
	{
		os_free(rcv_buf);
		rcv_buf = NULL;
	}

	if (db_udp_service->img_fd != -1)
	{
		close(db_udp_service->img_fd);
		db_udp_service->img_fd = -1;
	}

	if (db_udp_service->aud_fd != -1)
	{
		close(db_udp_service->aud_fd);
		db_udp_service->aud_fd = -1;
	}

	GLOBAL_INT_DISABLE();
	db_udp_service->img_status = 0;
	db_udp_service->running = 0;
	db_udp_service->aud_status = 0;
	GLOBAL_INT_RESTORE();

	db_udp_service->thd = NULL;
	rtos_delete_thread(NULL);
}


bk_err_t av_server_udp_service_init(void)
{
	int ret;

	LOGI("%s\n", __func__);


	if (db_udp_service != NULL)
	{
		LOGE("db_udp_service already init\n");
		return BK_FAIL;
	}

	ret = av_server_devices_init();

	if (ret != BK_OK)
	{
		LOGE("av_server_devices_init failed\n");
		goto error;
	}

	db_udp_service = os_malloc(sizeof(db_udp_service_t));

	if (db_udp_service == NULL)
	{
		LOGE("db_udp_service malloc failed\n");
		goto error;
	}

	os_memset(db_udp_service, 0, sizeof(db_udp_service_t));

	db_udp_service->img_channel = av_server_transmission_malloc(1600, AV_SERVER_NETWORK_MAX_SIZE);

	if (db_udp_service->img_channel == NULL)
	{
		LOGE("img_channel malloc failed\n");
		goto error;
	}

	db_udp_service->aud_channel = av_server_transmission_malloc(1600, AV_SERVER_NETWORK_MAX_SIZE);

	if (db_udp_service->aud_channel == NULL)
	{
		LOGE("aud_channel malloc failed\n");
		goto error;
	}

	av_server_devices_set_camera_transfer_callback(&av_server_udp_img_channel);

	av_server_devices_set_audio_transfer_callback(&av_server_udp_aud_channel);

#if 0//(!CONFIG_AV_DEMO_MODE_TCP)
	ret = rtos_init_semaphore(&s_av_server_service_sem, 1);
#endif

	ret = rtos_create_thread(&db_udp_service->thd,
							 4,
							 "av_server",
							 (beken_thread_function_t)av_server_udp_service_main,
							 1024 * 2,
							 (beken_thread_arg_t)NULL);
	if (ret != BK_OK)
	{
		LOGE("Error: Failed to create av server udp service: %d\n", ret);
		return BK_FAIL;
	}

	LOGI("db_udp_service->img_channel %p\n", db_udp_service->img_channel);
	LOGI("db_udp_service->aud_channel %p\n", db_udp_service->aud_channel);

	return BK_OK;
error:

	av_server_udp_service_deinit();

	return BK_FAIL;
}

void av_server_udp_service_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	LOGI("%s\n", __func__);

	if (db_udp_service == NULL)
	{
		LOGE("%s service is NULL\n", __func__);
		return;
	}

	if (db_udp_service->running)
	{
		av_server_camera_turn_off();
		av_server_audio_turn_off();
		av_server_display_turn_off();
	}

	av_server_devices_deinit();

	if (db_udp_service->aud_channel)
	{
		os_free(db_udp_service->aud_channel);
		db_udp_service->aud_channel = NULL;
	}

	if (db_udp_service->img_channel)
	{
		os_free(db_udp_service->img_channel);
		db_udp_service->img_channel = NULL;
	}

	GLOBAL_INT_DISABLE();
	db_udp_service->running == 0;
	GLOBAL_INT_RESTORE();

	while (db_udp_service->thd)
	{
		rtos_delay_milliseconds(10);
	}

	os_free(db_udp_service);
	db_udp_service = NULL;
}

