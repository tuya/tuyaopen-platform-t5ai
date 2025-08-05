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

#include <os/os.h>
#include "cli.h"
#include <driver/aon_rtc.h>
#include <sys_ctrl/sys_driver.h>
#include "aon_pmu_driver.h"

static void alarm_auto_test_callback(aon_rtc_id_t id, uint8_t *name_p, void *param);


static void cli_aon_rtc_help(void)
{
	CLI_LOGD("aon_rtc_get_time {id}\r\n");
	CLI_LOGD("aon_rtc_time_of_day {get|set} {sec|usec}\r\n");
}

static void cli_aon_rtc_get_time(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t aon_rtc_id;
	uint64_t tick = 0;

	aon_rtc_id = os_strtoul(argv[1], NULL, 10);
	tick = bk_aon_rtc_get_current_tick(aon_rtc_id)/bk_rtc_get_ms_tick_count();

	//CLI_LOGD("id=%d, tick_h=%d tick_l=%d\r\n", aon_rtc_id, (uint32_t)(tick>>32), (uint32_t)tick);
	CLI_LOGD("id=%d, tick_h=%d tick_l=%d ms\r\n", aon_rtc_id, (uint32_t)((tick)>>32), (uint32_t)(tick));
}

static void cli_aon_rtc_time_of_day(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_aon_rtc_help();
		return;
	}
	struct timeval tv;
	struct timezone tz;

	if (os_strcmp(argv[1], "get") == 0)
	{
		bk_rtc_gettimeofday(&tv, 0);
	}
	else if (os_strcmp(argv[1], "set") == 0)
	{
		tv.tv_sec = os_strtoul(argv[2], NULL, 10);
		tv.tv_usec = os_strtoul(argv[3], NULL, 10);
		bk_rtc_settimeofday(&tv, &tz);
	}
	else
		cli_aon_rtc_help();

}

#define AON_RTC_CMD_CNT (sizeof(s_aon_rtc_commands) / sizeof(struct cli_command))
static const struct cli_command s_aon_rtc_commands[] = {
	{"aon_rtc_get_time", "aon_rtc_get_time {id}", cli_aon_rtc_get_time},
	{"aon_rtc_time_of_day", "aon_rtc_time_of_day {get|set} {sec|usec}", cli_aon_rtc_time_of_day},
};

int cli_aon_rtc_init(void)
{
	return cli_register_commands(s_aon_rtc_commands, AON_RTC_CMD_CNT);
}

