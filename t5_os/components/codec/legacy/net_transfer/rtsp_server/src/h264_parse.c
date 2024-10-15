#include <os/mem.h>
#include <os/os.h>
#include "h264_parse.h"

#define TAG "H264-parse"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CHECK_HDR(ptr, i) ptr[i] == 0x0 && ptr[i + 1] == 0x0 \
						&& ptr[i + 2] == 0x0 && ptr[i + 3] == 0x1

#define H264_HEAD_LEN			256


int bk_h264_parse_head(uint8_t *h264_data, uint8_t *send_head, uint32_t *data_len)
{
	int head_ofs[8] = {0};
	int head_cnt = 0;

	for (int i = 0; i < H264_HEAD_LEN; i++)
	{
		if(CHECK_HDR(h264_data, i)) {
			head_ofs[head_cnt] = i;
			head_cnt ++;
		}
	}
	
	send_head[0] = (h264_data[4] & 0x60) | 0x18;

	if (head_cnt > 7 || head_cnt == 0) {
		LOGE("too much head count! %d \r\n", head_cnt);
		return -1;
	}

	if (head_cnt == 1) {
		LOGI("only data frame! %d \r\n", head_cnt);
		return 0;
	}
	
	int head_len = 1;
	for (int i = 0; i < head_cnt - 1; i++)
	{
		int temp_len = head_ofs[i + 1] - head_ofs[i];
		send_head[head_len] = ((temp_len-4) >> 8) & 0xFF;
		send_head[head_len + 1] = ((temp_len-4) >> 0) & 0xFF;
		head_len += 2;
		os_memcpy(send_head + head_len, h264_data + 4, temp_len - 4);
		h264_data += temp_len;
		head_len += temp_len - 4;
		*data_len += temp_len;
	}

	return head_len;
}