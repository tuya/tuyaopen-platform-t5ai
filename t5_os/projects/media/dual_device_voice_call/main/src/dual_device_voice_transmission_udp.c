#include <common/bk_include.h>
#include "lwip/tcp.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>

#include "lwip/sockets.h"

#include "voice_pipeline.h"
#include "dual_device_voice_transmission.h"


#define TAG "voice_udp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static int voice_udp_fd = -1;

static struct sockaddr_in *voice_udp_remote = NULL;

static uint8_t voice_udp_running = 0;

static beken_thread_t voice_udp_handle = NULL;

static voice_device_role_t device_role = VOICE_DEVICE_ROLE_CLIENT;

static int voice_udp_send_packet(unsigned char *data, unsigned int len)
{
	int send_byte = 0;
	send_byte = sendto(voice_udp_fd, data, len, 0, (struct sockaddr *)voice_udp_remote, sizeof(struct sockaddr_in));
	LOGD("len: %d, send_byte: %d \n", len, send_byte);
//	LOGI("sin_len: %d, sin_family: %d, sin_port: %04x, sin_addr: %08x \n", voice_udp_remote->sin_len, voice_udp_remote->sin_family, voice_udp_remote->sin_port, voice_udp_remote->sin_addr);
//send_byte = len;
	if (send_byte < len)
	{
		/* err */
		LOGE("need_send: %d, send_complete: %d, errno: %d \n", len, send_byte, errno);
		send_byte = 0;
	}

	return send_byte;
}

static void voice_udp_main(void *thread_param)
{
	uint32_t *buffer;
	int r_size;
	voice_setup_t voice_setup;
	int sender_len;
	static struct sockaddr_in server;

	buffer = os_malloc(TRANSFER_BUF_SIZE);
	if (buffer == NULL) {
		LOGE("buffer os_malloc failed\n");
		goto voice_udp_exit;
	}
	os_memset(buffer, 0x00, TRANSFER_BUF_SIZE);

	voice_udp_remote = (struct sockaddr_in *)os_malloc(sizeof(struct sockaddr_in));
	if (!voice_udp_remote)
	{
		LOGE("voice_udp_remote os_malloc failed\n");
		goto voice_udp_exit;
	}
	os_memset(voice_udp_remote, 0, sizeof(struct sockaddr_in));

	voice_udp_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (voice_udp_fd < 0) {
		LOGE("can't create socket!! exit\n");
		goto voice_udp_exit;
	}

	if (device_role == VOICE_DEVICE_ROLE_CLIENT) {
		voice_udp_remote->sin_family = PF_INET;
		voice_udp_remote->sin_port = htons(VOICE_PORT);
		voice_udp_remote->sin_addr.s_addr = inet_addr("192.168.0.1");
		LOGI("voice udp client run...\n");
	} else {
		server.sin_family = PF_INET;
		server.sin_port = htons(VOICE_PORT);
		server.sin_addr.s_addr = inet_addr("0.0.0.0");

		voice_udp_remote->sin_family = PF_INET;
		voice_udp_remote->sin_port = htons(VOICE_PORT);
		voice_udp_remote->sin_addr.s_addr = inet_addr("192.168.0.100");

		if (bind(voice_udp_fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
			LOGE("av_udp_server bind failed!! exit\n");
			goto voice_udp_exit;
		}
		LOGI("voice udp server run...\n");
	}

	if (device_role == VOICE_DEVICE_ROLE_CLIENT) {
		/* set timeout */
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		setsockopt(voice_udp_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	}

	voice_setup.voice_send_packet = voice_udp_send_packet;
	voice_init(voice_setup);

	voice_start();

	voice_udp_running = 1;

	while (voice_udp_running) {
		r_size = recvfrom(voice_udp_fd, buffer, TRANSFER_BUF_SIZE, 0, (struct sockaddr *)voice_udp_remote, (socklen_t *)&sender_len);
		if (r_size > 0) {
			LOGD("Rx data from client, r_size: %d \r\n", r_size);
//			LOGI("sin_len: %d, sin_family: %d, sin_port: %04x, sin_addr: %08x \n", voice_udp_remote->sin_len, voice_udp_remote->sin_family, voice_udp_remote->sin_port, voice_udp_remote->sin_addr);

			bk_err_t ret = voice_write_spk_data((char *)buffer, r_size);
			if (ret != r_size)
			{
				LOGE("write speaker data fial, size: %d, ret: %d\n", r_size, ret);
			}
		} else if (r_size == -1) {
			LOGI("recv timeout \n");
			continue;
		} else {
			// close this socket
			LOGI("recv close fd:%d, r_size:%d\n", voice_udp_fd, r_size);
			close(voice_udp_fd);
			voice_udp_fd = -1;
			goto voice_udp_exit;
		}
	}

voice_udp_exit:
	LOGI("%s, voice udp exit\r\n", __func__);

	/* close audio */
	voice_stop();
	voice_deinit();

	if (voice_udp_fd >= 0) {
		closesocket(voice_udp_fd);
	}

	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}

	if (voice_udp_remote)
	{
		os_free(voice_udp_remote);
		voice_udp_remote = NULL;
	}

	voice_udp_running = 0;
	voice_udp_handle = NULL;
	rtos_delete_thread(NULL);
}


static bk_err_t voice_udp_init(voice_device_role_t role)
{
	device_role = role;

	int ret = rtos_create_thread(&voice_udp_handle,
							 BEKEN_APPLICATION_PRIORITY,
							 "voice_udp",
							 (beken_thread_function_t)voice_udp_main,
							 THREAD_SIZE,
							 (beken_thread_arg_t)NULL);

	return ret;
}


bk_err_t dual_device_voice_transmission_udp_init(voice_device_role_t role)
{
	int ret;

	ret = voice_udp_init(role);
	if (ret != BK_OK)
	{
		LOGE("Error: create voice_task failed!\n");
		return ret;
	}

	return kNoErr;
}

bk_err_t dual_device_voice_transmission_udp_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	if (!voice_udp_running)
	{
		return BK_OK;
	}

	GLOBAL_INT_DISABLE();
	voice_udp_running = 0;
	GLOBAL_INT_RESTORE();

	while (voice_udp_handle)
	{
		rtos_delay_milliseconds(10);
	}

	return BK_OK;
}


