#include <common/bk_include.h>
#include "cli.h"
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include <string.h>
#include <common/sys_config.h>
#include <components/log.h>
#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>
#include <string.h>
#include "bk_wifi.h"
#include "av_client_comm.h"
#include "av_client_transmission.h"

#define TAG "db-comm"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


int av_client_socket_sendto(int *fd, const struct sockaddr *dst, uint8_t *data, uint32_t length, int offset, uint16_t *cnt)
{
	int ret = 0;

	uint8_t *ptr = data + offset;
	uint16_t size = length - offset;
	int max_retry = AV_CLIENT_SEND_MAX_RETRY;
	uint16_t index = 0;

	do {

		if (*fd < 0)
		{
			ret = -1;
			break;
		}

		ret = sendto(*fd, ptr + index, size - index, MSG_DONTWAIT | MSG_MORE,
	                   dst, sizeof(struct sockaddr_in));

		LOGD("send: %d, %d\n", ret, size);

		if (ret < 0)
		{
			ret = 0;
		}

		index += ret;

		if (index == size)
		{
			ret = size + offset;
			break;
		}

		max_retry--;

		if (max_retry < 0)
		{
			ret = -1;
			max_retry = 0;
			LOGE("reach max retry\n");
			break;
		}

		rtos_delay_milliseconds(AV_CLIENT_SEND_MAX_DELAY);

	} while (index < size);

	*cnt = AV_CLIENT_SEND_MAX_RETRY - max_retry;
	return ret;
}


