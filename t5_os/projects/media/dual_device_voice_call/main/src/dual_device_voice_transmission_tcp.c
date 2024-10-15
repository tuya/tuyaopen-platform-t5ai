#include <common/bk_include.h>
#include "lwip/tcp.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>
//#include <driver/lcd.h>

#include "lwip/sockets.h"

//#include <components/video_types.h>
//#include "aud_intf.h"

#include "dual_device_voice_transmission.h"

#define TAG "voice_tcp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static int voice_tcp_fd = -1;

static struct sockaddr_in *voice_tcp_remote = NULL;

static uint8_t voice_tcp_running = 0;

static beken_thread_t voice_tcp_handle = NULL;


static int voice_tcp_send_packet(unsigned char *data, unsigned int len)
{
	int send_byte = 0;
	send_byte = write(voice_tcp_fd, data, len);
	LOGD("len: %d, send_byte: %d \n", len, send_byte);
	if (send_byte < len)
	{
		/* err */
		LOGE("need_send: %d, send_complete: %d, errno: %d \n", len, send_byte, errno);
		send_byte = 0;
	}

	return send_byte;
}

static void voice_tcp_main(void *thread_param)
{
	bk_err_t ret = BK_OK;
	uint32_t *buffer;
	int r_size;
	voice_setup_t voice_setup;

	buffer = os_malloc(TRANSFER_BUF_SIZE);
	if (buffer == NULL)
		goto tcp_client_exit;
	os_memset(buffer, 0x00, TRANSFER_BUF_SIZE);

	voice_tcp_remote = (struct sockaddr_in *)os_malloc(sizeof(struct sockaddr_in));
	if (!voice_tcp_remote)
	{
		LOGE("voice_tcp_remote os_malloc failed\n");
		goto tcp_client_exit;
	}

	voice_setup.voice_send_packet = voice_tcp_send_packet;
	voice_init(voice_setup);

	LOGI("av_tcp_client start \n");

	while (1) {
		voice_tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (voice_tcp_fd < 0) {
			LOGE("av_tcp_client: create socket failed, err=%d!\n", errno);
		}

		voice_tcp_remote->sin_family = PF_INET;
		voice_tcp_remote->sin_port = htons(VOICE_PORT);
		voice_tcp_remote->sin_addr.s_addr = inet_addr("192.168.0.1");

		while (1) {
			ret = connect(voice_tcp_fd, (struct sockaddr *)voice_tcp_remote, sizeof(struct sockaddr_in));
			if(ret < 0)
			{
				LOGE("connect err: %d\r\n", ret);
				//goto tcp_client_exit;
				rtos_delay_milliseconds(500);
				continue;
			} else {
				LOGI("av_tcp_client connect complete \n");
				break;
			}
		}
		//LOGI("av_tcp_client connect complete \n");

		voice_start();
		LOGI("voice start... \n");

		voice_tcp_running = 1;

		while (voice_tcp_running) {
			r_size = recv(voice_tcp_fd, buffer, TRANSFER_BUF_SIZE, 0);
			if (r_size > 0) {
				LOGD("Rx data from server, r_size: %d \r\n", r_size);
				ret = bk_aud_intf_write_spk_data((uint8_t *)buffer, r_size);
				if (ret != BK_OK)
				{
					LOGE("write speaker data fial\n", r_size);
				}
			} else {
				// close this socket
				LOGI("recv close fd:%d, rcv_len:%d\n", voice_tcp_fd, r_size);
				close(voice_tcp_fd);
				voice_tcp_fd = -1;

				/* close audio */
				voice_stop();
				voice_deinit();

				goto tcp_client_exit;
			}
		}
	}

tcp_client_exit:
	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}

	if (voice_tcp_remote)
	{
		os_free(voice_tcp_remote);
		voice_tcp_remote = NULL;
	}

	if (voice_tcp_fd >= 0) {
		closesocket(voice_tcp_fd);
		voice_tcp_fd = -1;
	}

	voice_tcp_running = 0;

	voice_tcp_handle = NULL;
	rtos_delete_thread(NULL);
}


static bk_err_t voice_tcp_client_init(void)
{
	int ret = rtos_create_thread(&voice_tcp_handle,
							 BEKEN_APPLICATION_PRIORITY,
							 "voice_tcp_client",
							 (beken_thread_function_t)voice_tcp_main,
							 THREAD_SIZE,
							 (beken_thread_arg_t)NULL);

	return ret;
}

bk_err_t dual_device_voice_transmission_tcp_init(void)
{
	int ret;

	ret = voice_tcp_client_init();
	if (ret != BK_OK)
	{
		LOGE("Error: create voice_task failed!\n");
		return ret;
	}

	return kNoErr;
}

bk_err_t dual_device_voice_transmission_tcp_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	if (voice_tcp_running)
	{
		return BK_OK;
	}

	GLOBAL_INT_DISABLE();
	voice_tcp_running = 0;
	GLOBAL_INT_RESTORE();

	while (voice_tcp_handle)
	{
		rtos_delay_milliseconds(10);
	}

	return BK_OK;
}

