#include <os/os.h>

/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

#include "media_ipc.h"
#include "test/media_ipc_test.h"
#include "cli.h"

#define TAG "MIPC-UT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

meida_ipc_t ipc = NULL;

extern uint64_t bk_aon_rtc_get_us(void);




/*
* Pre set
*/

TEST_GROUP(TEST_IOT_MEDIA_IPC);

TEST_SETUP(TEST_IOT_MEDIA_IPC)
{
	//TODO
}

TEST_TEAR_DOWN(TEST_IOT_MEDIA_IPC)
{
	//TODO
}

/*
* Test Case : MediaIpcSync
*/

int media_ipc_sync_callback(uint8_t *data, uint32_t size, void *param)
{
	LOGI("%s, size: %d\n", __func__, size);

	return 0;
}


TEST(TEST_IOT_MEDIA_IPC, MediaIpcSync)
{
#if (CONFIG_SYS_CPU0)

	bk_err_t ret = BK_OK;
	media_ipc_chan_cfg_t cfg = {0};
	int i = 0;

	cfg.cb = media_ipc_sync_callback;
	cfg.name = "uvc";
	cfg.param = NULL;

	ret = media_ipc_channel_open(&ipc, &cfg);

	TEST_ASSERT_EQUAL(ret, BK_OK);

	for (i = 0; i < 100; i++)
	{
		char data[30] = {0};
		sprintf(data, "IPC TX Send Test: [%d]", i);

		uint64_t before = bk_aon_rtc_get_us();
		ret = media_ipc_send(&ipc, (uint8_t *)data, sizeof(data), MIPC_CHAN_SEND_FLAG_SYNC);
		uint64_t after = bk_aon_rtc_get_us();

		TEST_ASSERT_EQUAL(ret, BK_OK);

		LOGD("ipc send cost: %u\n", after - before);
	}

	ret = media_ipc_channel_close(&ipc);

	TEST_ASSERT_EQUAL(ret, BK_OK);

#endif
}

TEST(TEST_IOT_MEDIA_IPC, MediaIpcAsync)
{
#if (CONFIG_SYS_CPU0)

	bk_err_t ret = BK_OK;
	media_ipc_chan_cfg_t cfg = {0};
	int i = 0;

	cfg.cb = media_ipc_sync_callback;
	cfg.name = "uvc";
	cfg.param = NULL;

	ret = media_ipc_channel_open(&ipc, &cfg);

	TEST_ASSERT_EQUAL(ret, BK_OK);

	for (i = 0; i < 100; i++)
	{
		char data[30] = {0};
		sprintf(data, "IPC TX Send Test: [%d]", i);

		uint64_t before = bk_aon_rtc_get_us();
		ret = media_ipc_send(&ipc, (uint8_t *)data, sizeof(data), 0);
		uint64_t after = bk_aon_rtc_get_us();

		TEST_ASSERT_EQUAL(ret, BK_OK);

		LOGD("ipc send cost: %u\n", after - before);

		rtos_delay_milliseconds(100);
	}

	ret = media_ipc_channel_close(&ipc);

	TEST_ASSERT_EQUAL(ret, BK_OK);

#endif
}


TEST_GROUP_RUNNER(TEST_IOT_MEDIA_IPC)
{
	RUN_TEST_CASE(TEST_IOT_MEDIA_IPC, MediaIpcSync);
	RUN_TEST_CASE(TEST_IOT_MEDIA_IPC, MediaIpcAsync);
}

static void run_all_tests(void)
{
	RUN_TEST_GROUP(TEST_IOT_MEDIA_IPC);
}


#define MEDIA_TEST_CMD_CNT  (sizeof(s_media_ipc_test_commands) / sizeof(struct cli_command))

void cli_media_ipc_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UnityMain(0, NULL, run_all_tests);
}

static const struct cli_command s_media_ipc_test_commands[] =
{
	{"mipc", "media ipc...", cli_media_ipc_test_cmd},
};

int media_ipc_sync_cpu1_callback(uint8_t *data, uint32_t size, void *param)
{

	LOGI("size %d, %s\n", size, data);
	return 0;
}


int media_ipc_test_init(void)
{
#if (CONFIG_SYS_CPU1)
	bk_err_t ret;

	media_ipc_chan_cfg_t cfg = {0};
	cfg.cb = media_ipc_sync_cpu1_callback;
	cfg.name = "uvc";
	cfg.param = NULL;

	ret = media_ipc_channel_open(&ipc, &cfg);

	cli_register_commands(s_media_ipc_test_commands, MEDIA_TEST_CMD_CNT);

	return ret;
#else
	return cli_register_commands(s_media_ipc_test_commands, MEDIA_TEST_CMD_CNT);
#endif
}
