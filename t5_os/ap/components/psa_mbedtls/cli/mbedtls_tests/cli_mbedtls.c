// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"
#include "mbedtls_test.h"
#include <os/mem.h>
#include <driver/aon_rtc.h>
#include "sys_hal.h"
#include "modules/pm.h"
#include <os/os.h>

#define MBEDTLS_SHA_TEST_CNT    1
#define MBEDTLS_TEST_FREQUENCY  (PM_CPU_FRQ_240M)

static void cli_mbedtls_help(void)
{
	CLI_LOGD("mbedtls_sha 256/512\r\n");
	CLI_LOGD("mbedtls_aes ecb/cbc/ctr/gcm\r\n");
	CLI_LOGD("mbedtls_ecdsa [cnt]\r\n");
	CLI_LOGD("mbedtls_rsa\r\n");
	CLI_LOGD("mbedtls_selftest\r\n");
	CLI_LOGD("mbedtls_thread create [cnt]\r\n");
}

#define err_if(expr,status)                                          \
  do {                                                               \
    if (expr) {                                                      \
        CLI_LOGE("FAILED: %s(%d)(%d)\r\n",__func__,__LINE__,status); \
        err_cnt++;                                                   \
    }                                                                \
  } while(0)

const uint32_t test_len[] = {32, 1024, 4096, 32768};

static void cli_mbedtls_sha_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	uint32_t err_cnt = 0;
	int32_t ret = 0;

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	if (os_strcmp(argv[1], "256") == 0) {
		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_sha256_loop_test(test_len[i], MBEDTLS_SHA_TEST_CNT);
			err_if(ret != 0, ret);
		}
	}
	else if (os_strcmp(argv[1], "512") == 0) {

	}
	else {
		cli_mbedtls_help();
	}

	if (0 == err_cnt)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_aes_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	uint32_t err_cnt = 0;
	int ret = 0;
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	if (os_strcmp(argv[1], "ecb") == 0) {
		ret = te200_aes_ecb_test();
		err_if(ret != 0, ret);
	} 
	else if (os_strcmp(argv[1], "cbc") == 0) {
		ret = te200_aes_cbc_test();
		err_if(ret != 0, ret);

		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_aes_cbc_large_data_test(test_len[i]);
			err_if(ret != 0, ret);
		}
	} 
	else if (os_strcmp(argv[1], "ctr") == 0) {
	 	ret = te200_aes_ctr_test();
		err_if(ret != 0, ret);
	}
	else if (os_strcmp(argv[1], "gcm") == 0) {
		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_aes_gcm_large_data_test(test_len[i]);
			err_if(ret != 0, ret);
		}
	}
	else {
		cli_mbedtls_help();
	}

	if (0 == err_cnt)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_ecdsa_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	int ret = 0;
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	uint32_t loop_cnt = os_strtoul(argv[1], NULL, 10);
	ret = te200_ecdsa_self_test(1, loop_cnt);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_rsa_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	ret = te200_rsa_self_test(1);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_selftest(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	extern int mbedtls_selftest_main(int argc, char *argv[]);
	ret = mbedtls_selftest_main(argc, argv);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

#define    MBEDTLS_TEST_PRIORITY    4
static uint32_t g_max_count;
static void cli_mbedtls_thread(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	if (os_strcmp(argv[1], "create") == 0) {
		g_max_count = os_strtoul(argv[2], NULL, 10);
		BK_LOGD(NULL, "cli max counter %u\r\n", g_max_count);
		ret = rtos_create_thread(NULL,
							 MBEDTLS_TEST_PRIORITY,
							 "mbedtls_test",
							 (beken_thread_function_t)te200_muti_task_test,
							 1024*5,
							 (beken_thread_arg_t)(&g_max_count));
		if (ret != 0) {
			CLI_LOGE("Error: Failed to create mbedtls thread: %d\r\n", ret);
		}
	}else
	{
		cli_mbedtls_help();
	}
}

#define MBEDTLS_CMD_CNT (sizeof(s_mbedtls_commands) / sizeof(struct cli_command))
static const struct cli_command s_mbedtls_commands[] = {
	{"mbedtls_sha",      "mbedtls_sha {256|512}",         cli_mbedtls_sha_cmd},
	{"mbedtls_aes",      "mbedtls_aes {ecb|cbc|ctr|gcm}", cli_mbedtls_aes_cmd},
	{"mbedtls_ecdsa",    "mbedtls_ecdsa {10}",            cli_mbedtls_ecdsa_cmd},
	{"mbedtls_rsa",      "mbedtls_rsa",                   cli_mbedtls_rsa_cmd},
	{"mbedtls_selftest", "mbedtls_selftest",              cli_mbedtls_selftest},
	{"mbedtls_thread",   "mbedtls_thread {create}{count}",cli_mbedtls_thread},
};

int cli_mbedtls_init(void)
{
	return cli_register_commands(s_mbedtls_commands, MBEDTLS_CMD_CNT);
}
