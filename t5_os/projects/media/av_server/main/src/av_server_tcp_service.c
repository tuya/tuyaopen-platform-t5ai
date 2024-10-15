#include <common/bk_include.h>
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>

#include <driver/int.h>
#include <common/bk_err.h>
#include <getopt.h>

#include "lwip/udp.h"
#include "net.h"
#include "string.h"
#include <components/netif.h>
#include <common/bk_generic.h>
#include "av_server_comm.h"
#include "av_server_tcp_service.h"
#include "av_server_transmission.h"
#include "av_server_devices.h"
#include "lwip/sockets.h"
#include "wifi_transfer.h"
#include "media_app.h"
#include <driver/dvp_camera_types.h>

#include "cli.h"

#define TAG "db-tcp"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
	beken_thread_t img_thd;
	beken_thread_t aud_thd;
	struct sockaddr_in img_remote;
	struct sockaddr_in aud_remote;
	struct sockaddr_in img_socket;
	struct sockaddr_in aud_socket;
	int img_fd;
	int img_server_fd;
	int aud_fd;
	int aud_server_fd;
	uint16_t running : 1;
	uint16_t img_status : 1;
	uint16_t aud_status : 1;
	db_channel_t *img_channel;
	db_channel_t *aud_channel;
} db_tcp_service_t;

db_tcp_service_t *db_tcp_service = NULL;

camera_parameters_t camera_parameters = {UVC_DEVICE_ID,800,480,0,1,65535};
lcd_parameters_t lcd_parameters = {10,90,0};
audio_parameters_t audio_parameters = {1,0,1,1,8000,8000};


static struct sockaddr_in tcp_video_sender;
static struct sockaddr_in tcp_audio_sender;

int av_server_tcp_img_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	if (!db_tcp_service->img_status)
	{
		return -1;
	}

	return av_server_socket_sendto(&db_tcp_service->img_fd, (struct sockaddr *)&tcp_video_sender, data, len, -sizeof(db_trans_head_t), retry_cnt);
}

int av_server_tcp_img_send_prepare(uint8_t *data, uint32_t length)
{
	av_server_transmission_pack(db_tcp_service->img_channel, data, length);

	return 0;
}

void *av_server_tcp_img_get_tx_buf(void)
{
	if (db_tcp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return NULL;
	}

	if (db_tcp_service->img_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return NULL;
	}

	LOGI("%s, tbuf %p\n", __func__,db_tcp_service->img_channel->tbuf);

	return db_tcp_service->img_channel->tbuf + 1;
}

int av_server_tcp_img_get_tx_size(void)
{
	if (db_tcp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return 0;
	}

	if (db_tcp_service->img_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return 0;
	}

	return db_tcp_service->img_channel->tsize - sizeof(db_trans_head_t);
}


int av_server_tcp_aud_send_packet(uint8_t *data, uint32_t len, uint16_t *retry_cnt)
{
	if (!db_tcp_service->aud_status)
	{
		return -1;
	}

	return av_server_socket_sendto(&db_tcp_service->aud_fd, (struct sockaddr *)&tcp_audio_sender, data, len, -sizeof(db_trans_head_t), retry_cnt);
}

int av_server_tcp_aud_send_prepare(uint8_t *data, uint32_t length)
{
	av_server_transmission_pack(db_tcp_service->aud_channel, data, length);

	return 0;
}

void *av_server_tcp_aud_get_tx_buf(void)
{
	if (db_tcp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return NULL;
	}

	if (db_tcp_service->aud_channel == NULL)
	{
		LOGE("%s, aud_channel null\n",__func__);
		return NULL;
	}

	LOGD("%s, tbuf %p\n",__func__, db_tcp_service->aud_channel->tbuf);

	return db_tcp_service->aud_channel->tbuf + 1;
}

int av_server_tcp_aud_get_tx_size(void)
{
	if (db_tcp_service == NULL)
	{
		LOGE("%s, service null\n",__func__);
		return 0;
	}

	if (db_tcp_service->aud_channel == NULL)
	{
		LOGE("%s, img_channel null\n",__func__);
		return 0;
	}

	return db_tcp_service->aud_channel->tsize - sizeof(db_trans_head_t);
}

static media_transfer_cb_t av_server_tcp_img_channel =
{
	.send = av_server_tcp_img_send_packet,
	.prepare = av_server_tcp_img_send_prepare,
	.get_tx_buf = av_server_tcp_img_get_tx_buf,
	.get_tx_size = av_server_tcp_img_get_tx_size,
	.fmt = PIXEL_FMT_UNKNOW,
};

static const media_transfer_cb_t av_server_tcp_aud_channel =
{
	.send = av_server_tcp_aud_send_packet,
	.prepare = av_server_tcp_aud_send_prepare,
	.get_tx_buf = av_server_tcp_aud_get_tx_buf,
	.get_tx_size = av_server_tcp_aud_get_tx_size,
	.fmt = PIXEL_FMT_UNKNOW,
};

static inline void av_server_tcp_video_receiver(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
	//wifi_transfer_data_check(data,length);
#if (CONFIG_MEDIA_APP)
	wifi_transfer_net_send_data(data, length, TVIDEO_SND_TCP);
#endif

}

static void av_server_image_server_thread(beken_thread_arg_t data)
{
	int rcv_len = 0;
	//	struct sockaddr_in server;
	bk_err_t ret = BK_OK;
	u8 *rcv_buf = NULL;
	fd_set watchfd;

	LOGI("%s entry\n", __func__);
	(void)(data);

	rcv_buf = (u8 *) os_malloc((AV_SERVER_TCP_BUFFER + 1) * sizeof(u8));

	if (!rcv_buf)
	{
		LOGE("tcp os_malloc failed\n");
		goto out;
	}

	// for data transfer
	db_tcp_service->img_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (db_tcp_service->img_server_fd == -1)
	{
		LOGE("socket failed\n");
		goto out;
	}

	db_tcp_service->img_socket.sin_family = AF_INET;
	db_tcp_service->img_socket.sin_port = htons(AV_SERVER_TCP_IMG_PORT);
	db_tcp_service->img_socket.sin_addr.s_addr = inet_addr("0.0.0.0");

	tcp_video_sender.sin_family = AF_INET;
	tcp_video_sender.sin_port = htons(AV_SERVER_TCP_IMG_PORT);
	tcp_video_sender.sin_addr.s_addr = inet_addr("192.168.188.100");

	if (bind(db_tcp_service->img_server_fd, (struct sockaddr *)&db_tcp_service->img_socket, sizeof(struct sockaddr_in)) == -1)
	{
		LOGE("bind failed\n");
		goto out;
	}

	if (listen(db_tcp_service->img_server_fd, 0) == -1)
	{
		LOGE("listen failed\n");
		goto out;
	}

	LOGI("%s: start listen \n", __func__);

	while (1)
	{
		FD_ZERO(&watchfd);
		FD_SET(db_tcp_service->img_server_fd, &watchfd);

		LOGI("waiting for a new connection\n");
		ret = select(db_tcp_service->img_server_fd + 1, &watchfd, NULL, NULL, NULL);
		if (ret <= 0)
		{
			LOGE("select ret:%d\n", ret);
			continue;
		}
		else
		{
			// is new connection
			if (FD_ISSET(db_tcp_service->img_server_fd, &watchfd))
			{
				struct sockaddr_in client_addr;
				socklen_t cliaddr_len = 0;

				cliaddr_len = sizeof(client_addr);

				db_tcp_service->img_fd = accept(db_tcp_service->img_server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

				if (db_tcp_service->img_fd < 0)
				{
					LOGE("accept return fd:%d\n",db_tcp_service->img_fd);
					break;
				}

				LOGI("accept a new connection fd:%d\n", db_tcp_service->img_fd);

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

				db_tcp_service->img_status = BK_TRUE;

				while (db_tcp_service->img_status == BK_TRUE)
				{
					rcv_len = recv(db_tcp_service->img_fd, rcv_buf, AV_SERVER_TCP_BUFFER, 0);
					if (rcv_len > 0)
					{
						LOGD("got video length: %d\n", rcv_len);
						av_server_transmission_unpack(db_tcp_service->img_channel, rcv_buf, rcv_len, av_server_tcp_video_receiver);
					}
					else
					{
						// close this socket
						LOGI("vid recv close fd:%d, rcv_len:%d, error_code:%d\n", db_tcp_service->img_fd, rcv_len, errno);
						close(db_tcp_service->img_fd);
						db_tcp_service->img_fd = -1;

						break;
					}

				}
			}
		}
	}

out:

	LOGE("%s exit %d\n", __func__, db_tcp_service->img_status);

	av_server_tcp_service_deinit();

	if (rcv_buf)
	{
		os_free(rcv_buf);
		rcv_buf = NULL;
	}

	if (db_tcp_service->img_server_fd != -1)
	{
		close(db_tcp_service->img_server_fd);
		db_tcp_service->img_server_fd = -1;
	}

	db_tcp_service->img_status = BK_FALSE;

	db_tcp_service->img_thd = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t av_server_tcp_image_server_start(void)
{
	int ret = BK_FAIL;

	if (!db_tcp_service->img_thd)
	{
		ret = rtos_create_thread(&db_tcp_service->img_thd,
								 4,
								 "db_tcp_img_srv",
								 (beken_thread_function_t)av_server_image_server_thread,
								 1024 * 3,
								 (beken_thread_arg_t)NULL);
	}

	return ret;
}

static inline void av_server_tcp_voice_receiver(db_channel_t *channel, uint16_t sequence, uint16_t flags, uint32_t timestamp, uint8_t sequences, uint8_t *data, uint16_t length)
{
	LOGD("%s %d\n", __func__, length);
	av_server_audio_data_callback(data, length);
}


static void av_server_audio_server_thread(beken_thread_arg_t data)
{
	int rcv_len = 0;
	//	struct sockaddr_in server;
	bk_err_t ret = BK_OK;
	u8 *rcv_buf = NULL;
	fd_set watchfd;

	LOGI("%s entry\n", __func__);
	(void)(data);

	rcv_buf = (u8 *) os_malloc((AV_SERVER_TCP_BUFFER + 1) * sizeof(u8));

	if (!rcv_buf)
	{
		LOGE("tcp os_malloc failed\n");
		goto out;
	}

	// for data transfer
	db_tcp_service->aud_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (db_tcp_service->aud_server_fd == -1)
	{
		LOGE("socket failed\n");
		goto out;
	}

	db_tcp_service->aud_socket.sin_family = AF_INET;
	db_tcp_service->aud_socket.sin_port = htons(AV_SERVER_TCP_AUD_PORT);
	db_tcp_service->aud_socket.sin_addr.s_addr = inet_addr("0.0.0.0");

	tcp_audio_sender.sin_family = AF_INET;
	tcp_audio_sender.sin_port = htons(AV_SERVER_TCP_AUD_PORT);
	tcp_audio_sender.sin_addr.s_addr = inet_addr("192.168.188.100");

	if (bind(db_tcp_service->aud_server_fd, (struct sockaddr *)&db_tcp_service->aud_socket, sizeof(struct sockaddr_in)) == -1)
	{
		LOGE("bind failed\n");
		goto out;
	}

	if (listen(db_tcp_service->aud_server_fd, 0) == -1)
	{
		LOGE("listen failed\n");
		goto out;
	}

	LOGI("%s: start listen \n", __func__);

	while (1)
	{
		FD_ZERO(&watchfd);
		FD_SET(db_tcp_service->aud_server_fd, &watchfd);

		LOGI("waiting for a new connection\n");
		ret = select(db_tcp_service->aud_server_fd + 1, &watchfd, NULL, NULL, NULL);
		if (ret <= 0)
		{
			LOGE("select ret:%d\n", ret);
			continue;
		}
		else
		{
			// is new connection
			if (FD_ISSET(db_tcp_service->aud_server_fd, &watchfd))
			{
				struct sockaddr_in client_addr;
				socklen_t cliaddr_len = 0;

				cliaddr_len = sizeof(client_addr);

				db_tcp_service->aud_fd = accept(db_tcp_service->aud_server_fd, (struct sockaddr *)&client_addr, &cliaddr_len);

				if (db_tcp_service->aud_fd < 0)
				{
					LOGE("accept return fd:%d\n",db_tcp_service->aud_fd);
					break;
				}

				LOGI("accept a new connection fd:%d\n", db_tcp_service->aud_fd);

				ret = av_server_audio_turn_on(&audio_parameters);

				if (ret != BK_OK)
				{
					LOGE("turn on audio failed\n");
					goto out;
				}

				db_tcp_service->aud_status = BK_TRUE;

				av_server_socket_set_qos(db_tcp_service->aud_fd, IP_QOS_PRIORITY_HIGH);

				while (db_tcp_service->aud_status == BK_TRUE)
				{
					rcv_len = recv(db_tcp_service->aud_fd, rcv_buf, AV_SERVER_TCP_BUFFER, 0);
					if (rcv_len > 0)
					{
						LOGD("got audio length: %d\n", rcv_len);
						av_server_transmission_unpack(db_tcp_service->aud_channel, rcv_buf, rcv_len, av_server_tcp_voice_receiver);
					}
					else
					{
						// close this socket
						LOGI("aud recv close fd:%d, rcv_len:%d, error_code:%d\n", db_tcp_service->aud_fd, rcv_len, errno);
						close(db_tcp_service->aud_fd);
						db_tcp_service->aud_fd = -1;
						break;
					}
				}
			}
		}
	}

out:

	LOGE("%s exit %d\n", __func__, db_tcp_service->aud_status);

	av_server_tcp_service_deinit();

	if (rcv_buf)
	{
		os_free(rcv_buf);
		rcv_buf = NULL;
	}

	if (db_tcp_service->aud_server_fd != -1)
	{
		close(db_tcp_service->aud_server_fd);
		db_tcp_service->aud_server_fd = -1;
	}

	db_tcp_service->aud_status = BK_FALSE;

	db_tcp_service->aud_thd = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t doorbell_tcp_audio_server_start(void)
{
	int ret = BK_FAIL;

	if (!db_tcp_service->aud_thd)
	{
		ret = rtos_create_thread(&db_tcp_service->aud_thd,
								 4,
								 "db_tcp_aud_srv",
								 (beken_thread_function_t)av_server_audio_server_thread,
								 1024 * 3,
								 (beken_thread_arg_t)NULL);
	}

	return ret;
}

bk_err_t av_server_tcp_service_init(void)
{
	int ret;

	if (db_tcp_service != NULL)
	{
		LOGE("malloc db_tcp_service\n");
		goto error;
	}

	ret = av_server_devices_init();

	if (ret != BK_OK)
	{
		LOGE("av_server_devices_init failed\n");
		goto error;
	}

	db_tcp_service = os_malloc(sizeof(db_tcp_service_t));

	if (db_tcp_service == NULL)
	{
		LOGE("db_udp_service malloc failed\n");
		goto error;
	}

	os_memset(db_tcp_service, 0, sizeof(db_tcp_service_t));

	db_tcp_service->img_channel = av_server_transmission_malloc(AV_SERVER_TCP_NET_BUFFER, AV_SERVER_TCP_NET_BUFFER);

	if (db_tcp_service->img_channel == NULL)
	{
		LOGE("img_channel malloc failed\n");
		goto error;
	}

	db_tcp_service->aud_channel = av_server_transmission_malloc(AV_SERVER_TCP_NET_BUFFER, AV_SERVER_TCP_NET_BUFFER);

	if (db_tcp_service->aud_channel == NULL)
	{
		LOGE("aud_channel malloc failed\n");
		goto error;
	}

	av_server_devices_set_camera_transfer_callback(&av_server_tcp_img_channel);

	av_server_devices_set_audio_transfer_callback(&av_server_tcp_aud_channel);

	ret = av_server_tcp_image_server_start();

	if (ret != BK_OK)
	{
		LOGE("failed to create av_server tcp img server %d\n",ret);
		goto error;
	}

	ret = doorbell_tcp_audio_server_start();

	if (ret != BK_OK)
	{
		LOGE("failed to create av_server tcp aud server: %d\n",ret);
		goto error;
	}

	LOGI("db_tcp_service->img_channel %p\n", db_tcp_service->img_channel);
	LOGI("db_tcp_service->aud_channel %p\n", db_tcp_service->aud_channel);

	return BK_OK;

error:

	av_server_tcp_service_deinit();

	return BK_FAIL;

}

void av_server_tcp_service_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	LOGI("%s\n", __func__);

	if (db_tcp_service == NULL)
	{
		LOGE("%s service is NULL\n", __func__);
		return;
	}

	if (db_tcp_service->running)
	{
		av_server_camera_turn_off();
		av_server_audio_turn_off();
		av_server_display_turn_off();
	}

	av_server_devices_deinit();

	if (db_tcp_service->aud_channel)
	{
		os_free(db_tcp_service->aud_channel);
		db_tcp_service->aud_channel = NULL;
	}

	if (db_tcp_service->img_channel)
	{
		os_free(db_tcp_service->img_channel);
		db_tcp_service->img_channel = NULL;
	}

	GLOBAL_INT_DISABLE();
	db_tcp_service->running == 0;
	GLOBAL_INT_RESTORE();

	while (db_tcp_service->img_thd || db_tcp_service->aud_thd)
	{
		rtos_delay_milliseconds(10);
	}

	os_free(db_tcp_service);
	db_tcp_service = NULL;
}


