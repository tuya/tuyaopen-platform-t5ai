#include "bk_private/bk_init.h"
#include "bk_private/bk_wifi.h"
#include "modules/wifi_types.h"

#define CSI_TAG "csi"
#define CSI_LOGD(...) BK_LOGD(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGI(...) BK_LOGI(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGW(...) BK_LOGW(CSI_TAG, ##__VA_ARGS__)
#define CSI_LOGE(...) BK_LOGE(CSI_TAG, ##__VA_ARGS__)


#if (CONFIG_SYS_CPU0)

// Max csi info buffer
#define CSI_BUF_MAX  10

// Struct csi info buffer
typedef struct {
	uint8_t is_used;
	wifi_csi_info_t csi_buf;
}wifi_csi_info_buf_t;

//static const uint8_t CONFIG_CSI_SEND_MAC[] = {0x1a, 0x00, 0x00, 0x00, 0x00, 0x00};
static beken_semaphore_t csi_test_semaphore = NULL;
wifi_csi_info_buf_t csi_test_buf[CSI_BUF_MAX];
uint8_t csi_read_idx = 0;
uint8_t csi_write_idx = 0;

/**
 ****************************************************************************************
 * @brief csi_info_out
 * for user  getting csi info
 *
 * @param[in] info     Pointer to the csi info
 ****************************************************************************************
 */
void csi_info_out(wifi_csi_info_t *info)
{
	/// mac addr
	//if (os_memcmp(info->mac, CONFIG_CSI_SEND_MAC, WIFI_MAC_LEN)) {
	//	return;
	//}
	BK_LOG_RAW("SPCSIINFO %d \r\n",info->len);
	for(int i=0;i<info->len;i++)
	{
		BK_LOG_RAW("%x \r\n",info->buf[i]);
	}
	BK_LOG_RAW("EPCSIINFO \r\n");
}

/**
 ****************************************************************************************
 * @brief wifi_csi_rx_cb_demo
 * move csi info pointer to next 
 *
 * @param[in] info     Pointer to the csi info buffer
 * @param[in] flag     flag for getting buffer or write buffer finish
 ****************************************************************************************
 */
void wifi_csi_rx_cb_demo(wifi_csi_info_t **info,uint8_t flag)
{
	// get csi info buffer address
	if(flag == 0)
	{
		if(!csi_test_buf[csi_write_idx].is_used) {
			*info = &csi_test_buf[csi_write_idx].csi_buf;
		}
		else {
			CSI_LOGI("CSI ---buf full\r\n");
		}
	}
	else// csi info buffer write finsh
	{
		csi_test_buf[csi_write_idx].is_used = 1;
		csi_write_idx = (csi_write_idx + 1) % CSI_BUF_MAX;
		int ret = rtos_set_semaphore(&csi_test_semaphore);
		if (ret) {
			CSI_LOGI("CSI ---rtos_set_semaphore fail\r\n");
		}
	}
}

/**
 ****************************************************************************************
 * @brief csi_handler_main
 * csi info thread handler func
 *
 ****************************************************************************************
 */
static void csi_handler_main( UINT32 data )
{
	int ret;
	if(NULL == csi_test_semaphore)
	{
		ret = rtos_init_semaphore(&csi_test_semaphore, 32);
		if (kNoErr != ret)
			CSI_LOGI("CSI csi_test_semaphore init failed\r\n");
	}

	while (1)
	{
		ret = rtos_get_semaphore(&csi_test_semaphore, BEKEN_WAIT_FOREVER);
		if (ret) {
			CSI_LOGI("CSI csi_test_semaphore failed\r\n");
			break;
		}

		wifi_csi_info_buf_t *csi_info = &csi_test_buf[csi_read_idx];
		if (!csi_info || !csi_info->is_used) 
		{
			CSI_LOGW("wifi_csi_cb null,%d\r\n",csi_read_idx);
		}
		else
		{
			wifi_csi_info_t *info = &csi_info->csi_buf;

			// user data handler
			csi_info_out(info);

			csi_info->is_used = 0;
		}
		csi_read_idx = (csi_read_idx + 1) % CSI_BUF_MAX;
	}

	rtos_delete_thread(NULL);
}


int csi_thread_init(void)
{
	int ret;
	beken_thread_t csi_thread = NULL;

	ret = rtos_create_thread(&csi_thread,
								BEKEN_DEFAULT_WORKER_PRIORITY,
								"csi_info",
								(beken_thread_function_t)csi_handler_main,
								(1024 * 3),
								0);
	if (ret != kNoErr) {
		CSI_LOGE("Error: Failed to create at thread: %d\r\n",ret);
		goto csi_init_general_err;
	}
	return kNoErr;

csi_init_general_err:
	if( csi_thread != NULL ) {
		rtos_delete_thread(&csi_thread);
	}
	return kGeneralErr;
}

void wifi_csi_init(void)
{
	csi_thread_init();
	bk_wifi_csi_info_cb_register(wifi_csi_rx_cb_demo);
	//0x1:11g, 0x2:HT, 0x4:HE
	bk_wifi_set_csi_config(0x7);
	bk_wifi_csi_info_set(true);
}
#endif
int main(void)
{
	bk_init();
    #if (CONFIG_SYS_CPU0)
	wifi_csi_init();
    #endif
	return 0;
}
