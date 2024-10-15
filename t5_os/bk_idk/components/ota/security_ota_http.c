#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "_ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"
#include <driver/wdt.h>
#include <bk_wdt.h>
#include "sys_ctrl/sys_driver.h"
#include "security_ota.h"

#define TAG "ota"

u8  ota_flag =0;
int bk_http_ota_download(const char *uri)
{
	int ret;
	httpclient_t httpclient;
	httpclient_data_t httpclient_data;
	char http_content[HTTP_RESP_CONTENT_LEN];

	if(!uri)
	{
		ret = BK_FAIL;
		BK_LOGI(TAG,  "uri is NULL\r\n");
		return ret;
	}
	BK_LOGI(TAG, "http_ota_download :0x%x",bk_http_ota_download);

#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(1);
#endif

	ota_flag = 1;
	os_memset(&httpclient, 0, sizeof(httpclient_t));
	os_memset(&httpclient_data, 0, sizeof(httpclient_data));
	os_memset(&http_content, 0, sizeof(HTTP_RESP_CONTENT_LEN));
	httpclient.header = "Accept: text/xml,text/html,\r\n";
	httpclient_data.response_buf = http_content;
	httpclient_data.response_content_len = HTTP_RESP_CONTENT_LEN;
	ret = httpclient_common(&httpclient,
							uri,
							80,/*port*/
							NULL,
							HTTPCLIENT_GET,
							180000,
							&httpclient_data);


	ota_flag = 0;
	if (0 != ret)
	{
		BK_LOGI(TAG, "request epoch time from remote server failed.ret:%d\r\n",ret);
#if CONFIG_SYSTEM_CTRL
		bk_wifi_ota_dtim(0);
#endif
	}
	else
	{
		BK_LOGI(TAG, "sucess.\r\n");
		ret = security_ota_finish();
	}

	return ret;
}
