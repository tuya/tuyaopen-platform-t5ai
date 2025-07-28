#include "cli.h"
#include "modules/ota.h"
#if CONFIG_SECURITY_OTA
#include "_ota.h"
#endif

#if CONFIG_OTA_TFTP
extern void tftp_start(void);
extern void string_to_ip(char *s);
static void tftp_ota_get_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	short len = 0, i;
	extern char     BootFile[] ;

	if (argc > 3) {
		BK_LOGD(NULL,"ota server_ip ota_file\r\n");
		return;
	}

	BK_LOGD(NULL,"%s\r\n", argv[1]);

	os_strcpy(BootFile, argv[2]);
	BK_LOGD(NULL,"%s\r\n", BootFile);
	string_to_ip(argv[1]);


	tftp_start();

	return;

}
#endif

#if CONFIG_HTTP_AB_PARTITION
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	exec_flag ret_partition = 0;
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	ret_partition = bk_ota_get_current_partition();
	if(ret_partition == 0x0)
	{
    	BK_LOGD(NULL,"partition A\r\n");
    }
	else
	{
    	BK_LOGD(NULL,"partition B\r\n");
    }
#else
	ret_partition = bk_ota_get_current_partition();
	if((ret_partition == 0xFF) ||(ret_partition == EXEX_A_PART))
	{
    	BK_LOGD(NULL,"partition A\r\n");
    }
	else
	{
    	BK_LOGD(NULL,"partition B\r\n");
    }
#endif
}
#endif

#if CONFIG_DIRECT_XIP && CONFIG_SECURITY_OTA
void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern uint32_t flash_get_excute_enable();
	uint32_t id = flash_get_excute_enable();
	if(id == 0){
		BK_LOGD(NULL,"partition A\r\n");
	} else if (id == 1){
		BK_LOGD(NULL,"partition B\r\n");
	}
}
#endif

#if CONFIG_OTA_HTTP
void http_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;
	if (argc != 2)
		goto HTTP_CMD_ERR;
	ret = bk_http_ota_download(argv[1]);

	if (0 != ret)
		BK_LOGD(NULL,"http_ota download failed.");

	return;

HTTP_CMD_ERR:
	BK_LOGD(NULL,"Usage:http_ota [url:]\r\n");
}
#endif

#if CONFIG_OTA_HTTPS
char *https_url = NULL;
int bk_https_ota_download(const char *url);
void bk_https_start_download(beken_thread_arg_t arg) {
	int ret;
	ret = bk_https_ota_download(https_url);
	if(ret != BK_OK) {
		BK_LOGD(NULL,"%s download fail, ret:%d\r\n", __func__, ret);
	}
	rtos_delete_thread(NULL);
}

void https_ota_start(void)
{
	UINT32 ret;

	BK_LOGD(NULL,"https_ota_start\r\n");
	ret = rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
							 "https_ota",
							 (beken_thread_function_t)bk_https_start_download,
							 5120,
							 0);

	if (kNoErr != ret)
		BK_LOGD(NULL,"https_ota_start failed\r\n");

}

void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	if (argc != 2)
		goto HTTP_CMD_ERR;

	https_url = argv[1];
	https_ota_start();

	return;

HTTP_CMD_ERR:
	BK_LOGD(NULL,"%s,Usage:http_ota [url:]\r\n",__func__);
}
#endif

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
static const struct cli_command s_ota_commands[] = {

#if CONFIG_OTA_TFTP
	{"tftpota", "tftpota [ip] [file]", tftp_ota_get_Command},
#endif

#if CONFIG_OTA_HTTP
	{"http_ota", "http_ota url", http_ota_Command},
#endif

#if CONFIG_OTA_HTTPS
	{"https_ota", "ip [sta|ap][{ip}{mask}{gate}{dns}]", https_ota_Command},
	
#endif

#if CONFIG_HTTP_AB_PARTITION
	{"ab_version", NULL, get_http_ab_version},
#endif

#if CONFIG_DIRECT_XIP && CONFIG_SECURITY_OTA
	{"ab_version", NULL, get_http_ab_version},
#endif
};

#if (CONFIG_TFM_FWU)
extern int32_t ns_interface_lock_init(void);
#endif
int cli_ota_init(void)
{
#if (CONFIG_TFM_FWU)
	ns_interface_lock_init();
#endif
	return cli_register_commands(s_ota_commands, OTA_CMD_CNT);
}
