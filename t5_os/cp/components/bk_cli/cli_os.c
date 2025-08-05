#include "cli.h"
#include <os/os.h>
#include "bk_rtos_debug.h"
#include "stack_base.h"
#if (CONFIG_FREERTOS_TRACE)
#include "trcRecorder.h"
#endif

static void cli_task_list_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	rtos_dump_task_list();
}

static void cli_task_cpuload_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_FREERTOS
	rtos_dump_task_runtime_stats();
#endif
}

static void cli_task_backtrace_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	rtos_dump_backtrace();
}

static void cli_os_info_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	rtos_dump_backtrace();
	rtos_dump_task_list();

#if CONFIG_FREERTOS
	rtos_dump_task_runtime_stats();
#endif

#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
	os_dump_memory_stats(0, 0, NULL);
#endif

#if CONFIG_FREERTOS_V10
	extern void port_check_isr_stack(void);
	port_check_isr_stack();
#endif
}

static void cli_assert_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	BK_ASSERT(false);
}

#if (CONFIG_FREERTOS_TRACE)
static void cli_trace_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if (TRC_CFG_SCHEDULING_ONLY == 0)
	/* ==============User Event Sample 1 ===============*/
	TraceStringHandle_t chn1;
	xTraceStringRegister("Channel 1", &chn1);
	xTracePrint(chn1, "Begin");
	xTracePrintF(chn1, "User Event %d", 97);
	xTracePrint(chn1, "End");

	/* ==============User Event Sample 2 ===============*/
	TraceStringHandle_t chn2;
	TraceStringHandle_t fmt2;
	xTraceStringRegister("Channel 2", &chn2);
	xTraceStringRegister("Event!", &fmt2);

#if (TRC_CFG_USE_SEPARATE_USER_EVENT_BUFFER == 1)

	traceUBChannel UBCh2 = xTraceRegisterUBChannel(chn2, fmt2);
	// Result in "[Channel 2] Event!"
	vTraceUBEvent(UBCh2);


	/* ==============User Event Sample 3 ===============*/
	TraceStringHandle_t chn3;
	TraceStringHandle_t fmt3;
	xTraceStringRegister("Channel 3", &chn3);
	xTraceStringRegister("X: %d, Y: %d", &fmt3);
	traceUBChannel UBCh3 = xTraceRegisterUBChannel(chn3, fmt3);

	// Result in "[Channel 2] X: 23, Y: 19"
	vTraceUBData(UBCh3, 23, 19);

#endif //#if (TRC_CFG_USE_SEPARATE_USER_EVENT_BUFFER == 1)

	/* You can also use the other user event functions, like xTracePrintF.
	 * as they are then rerouted to the UB instead of the main event buffer.
	 * vTracePrintF then looks up the correct UB channel based on the
	 * provided channel name and format string, or creates a new UB channel
	 * if no match is found. The format string should therefore not contain
	 * "random" messages but mainly format specifiers. Random strings should
	 * be stored using %s and with the string as an argument.
	 *
	 */
	// Creates a new UB channel ("Channel 2", "%Z: %d")
	xTracePrintF(chn2, "aaaaa: %d", 10);
	// Finds the existing UB channel
	xTracePrintF(chn2, "bbbbb: %d", 24);
#endif //#if (TRC_CFG_SCHEDULING_ONLY == 0)
	uint32_t trace_addr = (uint32_t)xTraceGetTraceBuffer();
	uint32_t trace_size = uiTraceGetTraceBufferSize();

	stack_mem_dump(trace_addr, trace_addr + trace_size);
}
#endif
#if CONFIG_DEBUG_VERSION
void os_test_stack_guard(uint32_t override_len)
{
	unsigned char test_buf[32] = {0};
	volatile unsigned char *p_test = (volatile unsigned char *)test_buf;
	for (int i = 0; i < 32 + override_len; ++i)
	{
		/* code */
		*p_test = 0xff - i;
		p_test++;
	}

	return;
}
#endif

static void cli_test_stack_guard_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_DEBUG_VERSION
    uint32_t override_len = 16;
    BK_LOGD(NULL,"Test stack guard beging.\r\n");
    if (argc >= 2) {
        override_len = strtoll(argv[1], NULL, 16);
        BK_LOGD(NULL,"Test stack guard, override_len: %d.\r\n", override_len);

        os_test_stack_guard(override_len);
    } else {
        BK_LOGD(NULL,"stackguard <override_len>\r\n");
    }
#endif
}

#if CONFIG_FREERTOS

static uint32_t s_xtop_enable = 0;
static beken_thread_t s_xtop_thread = NULL;

static void xtop_cpu_percentage( void *arg )
{
	uint32_t timeToDump = (uint32_t)arg;
	uint32_t timeEnum;
	if (timeToDump == 1) {
		timeEnum = 0;
	} else if (timeToDump == 5) {
		timeEnum = 1;
	} else if (timeToDump == 10) {
		timeEnum = 2;
	} else {
		os_printf("time = %u\r\n", timeToDump);
		return;
	}

	while (s_xtop_enable) {
		rtos_dump_task_history_runtime_stats(timeEnum);
		rtos_delay_milliseconds(timeToDump * 1000);
	}
	s_xtop_thread = NULL;
	rtos_delete_thread(NULL);
}

static void start_xtop_task(uint32_t timeToDump)
{
	bk_err_t ret;
	s_xtop_enable = 1;
	ret = rtos_create_thread(&s_xtop_thread,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "xtop",
							 (beken_thread_function_t)xtop_cpu_percentage,
							 1024,
							 (void *)timeToDump);
	
	(void)ret;
}
#endif

static void cli_task_xtop_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_FREERTOS
	if (s_xtop_enable == 1 || s_xtop_thread != NULL) {
		os_printf("top cmd has not end\r\n");
		return;
	}
	uint32_t timeToDump;
	if (argc >= 2) {
        timeToDump = strtoll(argv[1], NULL, 10);
		if (timeToDump != 1 && timeToDump != 5 && timeToDump != 10) {
			return;
		}
    } else {
        os_printf("\r\n");
		return;
    }
	start_xtop_task(timeToDump);
#endif
}

static void cli_task_end_xtop_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_FREERTOS
	s_xtop_enable = 0;
#endif
}


#define OS_CMD_CNT (sizeof(s_os_commands) / sizeof(struct cli_command))
static const struct cli_command s_os_commands[] = {
	{"tasklist", "list tasks", cli_task_list_cmd},
	{"cpuload", "show task cpu load", cli_task_cpuload_cmd},
	{"backtrace", "show task backtrace", cli_task_backtrace_cmd},
	{"osinfo", "show os runtime information", cli_os_info_cmd},
	{"assert", "asset and dump system information", cli_assert_dump_cmd},
	{"stackguard", "stackguard <override_len>", cli_test_stack_guard_cmd},
#if (CONFIG_FREERTOS_TRACE)
	{"trace", "test trace information", cli_trace_cmd},
#endif
#if CONFIG_FREERTOS_HISTORY_CPU_PERCENT
	{"ostop", "xtop {1|5|10}", cli_task_xtop_cmd},
	{"ostop_end", "terminate top cmd", cli_task_end_xtop_cmd},
#endif
};

int cli_os_init(void)
{
	return cli_register_commands(s_os_commands, OS_CMD_CNT);
}
