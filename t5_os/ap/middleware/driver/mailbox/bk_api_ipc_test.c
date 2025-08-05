#include <os/os.h>

/* Test includes. */
#include "unity_fixture.h"
#include "unity.h"

#include "bk_api_ipc.h"
#include "bk_api_ipc_test.h"
#include "cli.h"

#define TAG "MIPC-UT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern uint64_t bk_aon_rtc_get_us(void);
extern uint32_t xTaskGetTickCount(void);

BK_IPC_CHANNEL_DEF(ui_ipc_test);

/*
* Pre set
*/

TEST_GROUP(TEST_IOT_AVDK_IPC);

TEST_SETUP(TEST_IOT_AVDK_IPC)
{
    //TODO
}

TEST_TEAR_DOWN(TEST_IOT_AVDK_IPC)
{
    //TODO
}

/*
* Test Case : AVDKIpcSync
*/


TEST(TEST_IOT_AVDK_IPC, AVDKIpcSync)
{
    bk_err_t ret = BK_OK;
    int i = 0;

    TEST_ASSERT_EQUAL(ret, BK_OK);

    char data[128] = {0};

    for (i = 0; i < 5; i++)
    {
        uint64_t before = bk_aon_rtc_get_us();

        sprintf(data, "Sync Hello From CPU: %d, @time: %lu", bk_ipc_cpu_id_get(), before);

        ret = bk_ipc_send(&ui_ipc_test, (uint8_t *)data, sizeof(data), MIPC_CHAN_SEND_FLAG_SYNC, 0);
        uint64_t after = bk_aon_rtc_get_us();

        TEST_ASSERT_EQUAL(ret, BK_OK);

        LOGV("ipc send cost: %u\n", after - before);
    }

    TEST_ASSERT_EQUAL(ret, BK_OK);
}

TEST(TEST_IOT_AVDK_IPC, AVDKIpcAsync)
{
    bk_err_t ret = BK_OK;
    int i = 0;

    TEST_ASSERT_EQUAL(ret, BK_OK);

    char data[128] = {0};

    for (i = 0; i < 5; i++)
    {
        uint64_t before = bk_aon_rtc_get_us();

        sprintf(data, "Async Hello From CPU: %d, @time: %lu", bk_ipc_cpu_id_get(), before);

        ret = bk_ipc_send(&ui_ipc_test, (uint8_t *)data, sizeof(data), 0, 0);
        uint64_t after = bk_aon_rtc_get_us();

        TEST_ASSERT_EQUAL(ret, BK_OK);

        LOGV("ipc send cost: %u\n", after - before);

        rtos_delay_milliseconds(100);
    }

    TEST_ASSERT_EQUAL(ret, BK_OK);
}


TEST_GROUP_RUNNER(TEST_IOT_AVDK_IPC)
{
    RUN_TEST_CASE(TEST_IOT_AVDK_IPC, AVDKIpcSync);
    RUN_TEST_CASE(TEST_IOT_AVDK_IPC, AVDKIpcAsync);
}

static void run_all_tests(void)
{
    RUN_TEST_GROUP(TEST_IOT_AVDK_IPC);
}


#define MEDIA_TEST_CMD_CNT  (sizeof(s_bk_ipc_test_commands) / sizeof(struct cli_command))

void cli_bk_ipc_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    UnityMain(argc, (const char **)argv, run_all_tests);
}

static const struct cli_command s_bk_ipc_test_commands[] =
{
    {"ipc", "user ipc...", cli_bk_ipc_test_cmd},
};


uint32_t bk_ipc_sync_cpu1_callback(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj)
{
    LOGD("Rx: [%s], size: %d\n", data, size);

    char buffer[128] = {0};
    sprintf(buffer, "Ack From CPU: %d, @time: %u", bk_ipc_cpu_id_get(), xTaskGetTickCount());

    bk_err_t ret = bk_ipc_send(&ui_ipc_test, (uint8_t *)buffer, sizeof(buffer), MIPC_CHAN_SEND_FLAG_SYNC, 0);

    return ret;
}

BK_IPC_CHANNEL_REGISTER(ui_ipc_test, IPC_ROUTE_CPU0_CPU1, bk_ipc_sync_cpu1_callback, NULL, NULL);



int bk_ipc_test_init(void)
{
    return cli_register_commands(s_bk_ipc_test_commands, MEDIA_TEST_CMD_CNT);
}
