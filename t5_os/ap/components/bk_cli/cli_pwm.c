#include "cli.h"
#include <os/os.h>
#include <driver/pwm.h>
#include "pwm_hal.h"

#if CONFIG_PWM

#define _PERIOD_2_FREQ(period)    ((period == 0) ? (0) : (26000000 / (period)))

static void cli_pwm_help(void)
{
	CLI_LOGD("pwm_driver init\n");
	CLI_LOGD("pwm_driver deinit\n");
	CLI_LOGD("pwm {chan} init {period_v} {duty_v} [duty1_v][duty2_v][duty3_v]\n");
	CLI_LOGD("pwm {chan} {start|stop|deinit}\n");
	CLI_LOGD("pwm {chan} duty {period_v} {duty1_v} [duty2_v] [duty3_v]\n");
	CLI_LOGD("pwm {chan} signal {low|high}\n");
	CLI_LOGD("pwm_group init {chan1} {chan2} {period} {chan1_duty} {chan2_duty}\n");
	CLI_LOGD("pwm_group {start|stop|deinit}\n");
	CLI_LOGD("pwm_group config {group} {period} {chan1_duty} {chan2_duty}\n");
	CLI_LOGD("pwm_int {chan} {reg|enable|disable}\n");
	CLI_LOGD("pwm_capture {chan} {init}\nn");
	CLI_LOGD("pwm_capture {chan} {start|stop|deinit}\n");
	CLI_LOGD("pwm_idle_test {idle_start|idle_stop}\n");
}

static void cli_pwm_timer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_counter_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_carrier_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
}

static void cli_pwm_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_pwm_driver_init());
		CLI_LOGD("pwm init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_pwm_driver_deinit());
		CLI_LOGD("pwm deinit\n");
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		pwm_init_config_t config = {0};

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		config.psc = os_strtoul(argv[7], NULL, 10);

		BK_LOG_ON_ERR(bk_pwm_init(chan, &config));
		CLI_LOGD("pwm init, chan=%d period=%x duty=%x\n", chan, config.period_cycle, config.duty_cycle);
	} else if (os_strcmp(argv[2], "start") == 0) {
		BK_LOG_ON_ERR(bk_pwm_start(chan));
		CLI_LOGD("pwm start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_stop(chan));
		CLI_LOGD("pwm stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_pwm_deinit(chan));
		CLI_LOGD("pwm deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "signal") == 0) {
		if (argc != 4) {
			cli_pwm_help();
			return;
		}

		if (os_strcmp(argv[3], "low") == 0)
			BK_LOG_ON_ERR(bk_pwm_set_init_signal_low(chan));
		else
			BK_LOG_ON_ERR(bk_pwm_set_init_signal_high(chan));
		CLI_LOGD("pwm set signal, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "duty") == 0) {
		pwm_period_duty_config_t config = {0};
		if (argc < 5) {
			cli_pwm_help();
			return;
		}

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.duty_cycle = os_strtoul(argv[4], NULL, 10);
		if (argc > 5)
			config.duty2_cycle = os_strtoul(argv[5], NULL, 10);
		if (argc > 6)
			config.duty3_cycle = os_strtoul(argv[6], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_set_period_duty(chan, &config));
		CLI_LOGD("pwm duty, chan=%d period=%x t1=%x t2=%x t3=%x\n", chan, config.period_cycle,
				 config.duty_cycle, config.duty2_cycle, config.duty3_cycle);
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_group_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t group;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	group = os_strtoul(argv[2], NULL, 10);

	if (os_strcmp(argv[1], "init") == 0) {
		if (argc != 7) {
			cli_pwm_help();
			return;
		}
		pwm_group_init_config_t config = {0};
		pwm_group_t group = 0;

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		config.period_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[5], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[6], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_init(&config, &group));
		CLI_LOGD("pwm init, group=%d chan1=%d chan2=%d period=%x d1=%x d2=%x\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "start") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_start(group));
		CLI_LOGD("pwm start, group=%d\n", group);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_stop(group));
		CLI_LOGD("pwm stop, group=%d\n", group);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_deinit(group));
		CLI_LOGD("pwm deinit, group=%d\n", group);
	} else if (os_strcmp(argv[1], "config") == 0) {
		pwm_group_config_t config = {0};
		if (argc != 6) {
			cli_pwm_help();
			return;
		}

		config.period_cycle = os_strtoul(argv[3], NULL, 10);
		config.chan1_duty_cycle = os_strtoul(argv[4], NULL, 10);
		config.chan2_duty_cycle = os_strtoul(argv[5], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_set_config(group, &config));
		CLI_LOGD("pwm config, group=%x period=%x chan1_duty=%x chan2_duty=%x\n",
				 group, config.period_cycle, config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else {
		cli_pwm_help();
		return;
	}
}


static void cli_pwm_isr(pwm_chan_t chan)
{
	CLI_LOGD("isr(%d)\n", chan);
}

static void cli_pwm_capture_isr(pwm_chan_t chan)
{
	CLI_LOGD("capture(%d), value=%x\n", chan, bk_pwm_capture_get_value(chan));
}

static void cli_pwm_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc != 3) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "reg") == 0) {
		BK_LOG_ON_ERR(bk_pwm_register_isr(chan, cli_pwm_isr));
		CLI_LOGD("pwm chan%d register interrupt isr\n", chan);
	} else if (os_strcmp(argv[2], "enable") == 0) {
		BK_LOG_ON_ERR(bk_pwm_enable_interrupt(chan));
		CLI_LOGD("pwm chan%d enable interrupt\n", chan);
	} else if (os_strcmp(argv[2], "disable") == 0) {
		BK_LOG_ON_ERR(bk_pwm_disable_interrupt(chan));
		CLI_LOGD("pwm chan%d disable interrupt\n", chan);
	} else {
		cli_pwm_help();
		return;
	}
}

static void cli_pwm_capture_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t chan;

	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	chan = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		pwm_capture_init_config_t config = {0};
		if (os_strcmp(argv[3], "pos") == 0)
			config.edge = PWM_CAPTURE_POS;
		else if (os_strcmp(argv[3], "neg") == 0)
			config.edge = PWM_CAPTURE_NEG;
		else if (os_strcmp(argv[3], "edge") == 0)
			config.edge = PWM_CAPTURE_EDGE;
		else {
			cli_pwm_help();
			return;
		}

		config.isr = cli_pwm_capture_isr;
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		CLI_LOGD("pwm_capture init, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "start") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));
		CLI_LOGD("pwm_capture start, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		CLI_LOGD("pwm_capture stop, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
		CLI_LOGD("pwm_capture deinit, chan=%d\n", chan);
	} else if (os_strcmp(argv[2], "capture_example") == 0) {
		pwm_capture_init_config_t config = {0};
		float duty_ratio = 0;

		config.edge = PWM_CAPTURE_POS;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));

		uint32_t period_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		CLI_LOGD("pwm_capture period_cycle:%d, freq:%dHz\n", period_cycle, _PERIOD_2_FREQ(period_cycle));
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));

		config.edge = PWM_CAPTURE_EDGE;
		config.isr = NULL;
		BK_LOG_ON_ERR(bk_pwm_capture_init(chan, &config));
		BK_LOG_ON_ERR(bk_pwm_capture_start(chan));
		uint32_t duty_cycle = bk_pwm_capture_get_period_duty_cycle(chan, 1000);
		if (period_cycle == 0) {
			duty_ratio = 0;
		} else {
			duty_ratio = (float)duty_cycle / (float)period_cycle;
		}
		CLI_LOGD("pwm_capture duty_cycle:%d, duty_ratio:%f\r\n", duty_cycle, duty_ratio);
		BK_LOG_ON_ERR(bk_pwm_capture_stop(chan));
		BK_LOG_ON_ERR(bk_pwm_capture_deinit(chan));
	} else {
		cli_pwm_help();
		return;
	}
}

#ifndef PWM_CLOCK_SRC_XTAL
#define PWM_CLOCK_SRC_XTAL 26000000
#endif
#define PWM_FREQ           (16000)
static uint32_t s_period_cycle = PWM_CLOCK_SRC_XTAL / PWM_FREQ;

static void pwm_init_and_start_all_chan_test(uint32_t period_cycle, uint32_t duty_cycle)
{
	BK_LOG_ON_ERR(bk_pwm_driver_init());

	pwm_init_config_t init_config = {0};

	for(pwm_chan_t chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
		init_config.period_cycle = period_cycle;
		init_config.duty_cycle = duty_cycle;
		BK_LOG_ON_ERR(bk_pwm_init(chan, &init_config));
		BK_LOG_ON_ERR(bk_pwm_start(chan));
	}
	s_period_cycle = period_cycle;
	rtos_delay_milliseconds(1000);

	CLI_LOGD("pwm_init_and_start period=%d duty=%d\r\n", init_config.period_cycle, init_config.duty_cycle);
}

static void pwm_update_all_chan_test(void)
{
	pwm_period_duty_config_t pwm_config = {0};
	uint32_t pwm_step = s_period_cycle / 10;
	uint32_t i = 0;
	uint32_t j = 0;
	pwm_chan_t chan = 0;

	for (j = 0; j < 5; j++) {
		CLI_LOGD("turn on\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * i;
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
			chan = 0;
		}

		CLI_LOGD("turn off\r\n");
		for (i = 0; i < 11; i++) {
			for(chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				pwm_config.period_cycle = s_period_cycle;
				pwm_config.duty_cycle = pwm_step * (10 - i);
				pwm_config.duty2_cycle = 0;
				pwm_config.duty3_cycle = 0;
				pwm_config.psc = 0;
				bk_pwm_set_period_duty(chan, &pwm_config);
			}
			rtos_delay_milliseconds(100);
		}
	}
}

#if CONFIG_PWM_PHASE_SHIFT
#define PHASE_SHIFT_CONFIG { \
	.psc = 0, \
	.chan_num = 6,\
	.period_cycle = s_period_cycle, \
	.duty_config[0] = { \
		.chan = 0, \
		.duty_cycle = 0, \
	}, \
	.duty_config[1] = { \
		.chan = 1, \
 		.duty_cycle = 0, \
	}, \
	.duty_config[2] = { \
		.chan = 2, \
		.duty_cycle = 0, \
	}, \
	.duty_config[3] = { \
		.chan = 3, \
		.duty_cycle = 0, \
	}, \
	.duty_config[4] = { \
		.chan = 4, \
		.duty_cycle = 0, \
	}, \
	.duty_config[5] = { \
		.chan = 5, \
		.duty_cycle = 0, \
	}, \
} \

static int pwm_phase_shift_test(void)
{
	pwm_phase_shift_config_t config = PHASE_SHIFT_CONFIG;

	BK_LOG_ON_ERR(bk_pwm_driver_init());
	BK_LOG_ON_ERR(bk_pwm_phase_shift_init(&config));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_start());

	uint32_t rand_num = 0;
	time_t time_num;

	srand((unsigned)time(&time_num));

	for (int j = 0; j < 10; j++) {
		CLI_LOGD("phase_shift test_num:%d\r\n", j);
		for (int i = 0; i < config.chan_num; i++) {
			rand_num = rand() % 11;
			CLI_LOGD("phase_shift rand_num:%d\r\n", rand_num);
			BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(config.duty_config[i].chan, s_period_cycle * 0.1f * rand_num));
		}
		BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());
		rtos_delay_milliseconds(10);
		BK_LOGD(NULL, "\r\n");
	}

	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(0, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(1, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(2, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(3, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(4, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_set_duty(5, 0));
	BK_LOG_ON_ERR(bk_pwm_phase_shift_update_duty());

	CLI_LOGD("phase_shift test over\r\n");

	return 0;
}
#endif

static void cli_pwm_idle_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_pwm_help();
		return;
	}

	pwm_group_t group = 0;

	if (os_strcmp(argv[1], "idle_init") == 0) {
		uint32_t freq = os_strtoul(argv[2], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;

		pwm_init_and_start_all_chan_test(period_cycle, 0);
		pwm_init_and_start_all_chan_test(period_cycle, period_cycle);
		pwm_init_and_start_all_chan_test(period_cycle, 1);
		pwm_init_and_start_all_chan_test(period_cycle, period_cycle - 2);
		pwm_init_and_start_all_chan_test(period_cycle, period_cycle * 0.2f);

		CLI_LOGD("pwm idle_init done\r\n");
	} else if (os_strcmp(argv[1], "idle_start") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
				BK_LOG_ON_ERR(bk_pwm_start(chan));
		}
	} else if (os_strcmp(argv[1], "idle_stop") == 0) {
		for(int chan = 0; chan < SOC_PWM_CHAN_NUM_PER_UNIT * SOC_PWM_UNIT_NUM; chan++) {
			BK_LOG_ON_ERR(bk_pwm_stop(chan));
		}
	} else if (os_strcmp(argv[1], "idle_update") == 0) {
		pwm_update_all_chan_test();
	} else if (os_strcmp(argv[1], "group_init") == 0) {
		BK_LOG_ON_ERR(bk_pwm_driver_init());
		pwm_group_init_config_t config = {0};

		config.chan1 = os_strtoul(argv[2], NULL, 10);
		config.chan2 = os_strtoul(argv[3], NULL, 10);
		uint32_t freq = os_strtoul(argv[4], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_init(&config, &group));

		CLI_LOGD("pwm group=%d chan1=%d chan2=%d period=%d d1=%d d2=%d\n",
				 group, config.chan1, config.chan2, config.period_cycle,
				 config.chan1_duty_cycle, config.chan2_duty_cycle);
	} else if (os_strcmp(argv[1], "group_start") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_pwm_group_start(start_group));
	} else if (os_strcmp(argv[1], "group_stop") == 0) {
		BK_LOG_ON_ERR(bk_pwm_group_stop(group));
	} else if (os_strcmp(argv[1], "group_update") == 0) {
		uint32_t start_group = os_strtoul(argv[2], NULL, 10);
		uint32_t freq = os_strtoul(argv[3], NULL, 10);
		uint32_t period_cycle = PWM_CLOCK_SRC_XTAL / freq;

		pwm_group_config_t config = {0};

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.3f;
		config.chan2_duty_cycle = period_cycle * 0.3f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);

		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = 0;
		config.chan2_duty_cycle = period_cycle;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle;
		config.chan2_duty_cycle = 0;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
		rtos_delay_milliseconds(1000);
		config.period_cycle = period_cycle;
		config.chan1_duty_cycle = period_cycle * 0.2f;
		config.chan2_duty_cycle = period_cycle * 0.4f;
		BK_LOG_ON_ERR(bk_pwm_group_set_config(start_group, &config));
	}
#if CONFIG_PWM_PHASE_SHIFT
	else if(os_strcmp(argv[1], "phase_shift") == 0) {
		pwm_phase_shift_test();
	}
#endif
	else {
		cli_pwm_help();
		return;
	}
}

#define PWM_CMD_CNT (sizeof(s_pwm_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwm_commands[] = {
	{"pwm_driver", "{init|deinit}}", cli_pwm_driver_cmd},
	{"pwm", "pwm {chan} {config|start|stop|init|deinit|signal} [...]", cli_pwm_cmd},
	{"pwm_int", "pwm_int {chan} {reg|enable|disable}", cli_pwm_int_cmd},
	//{"pwm_duty", "pwm_duty {chan} {period} {d1} [d2] [d3]", cli_pwm_cmd},
	{"pwm_capture", "pwm_capture {chan} {config|start|stop|init|deinit}", cli_pwm_capture_cmd},
	{"pwm_group", "pwm_group {init|deinit|config|start|stop} [...]", cli_pwm_group_cmd},
	{"pwm_timer", "pwm_timer ", cli_pwm_timer_cmd},
	{"pwm_counter", "pwm_counter", cli_pwm_counter_cmd},
	{"pwm_carrier", "pwm_carrier", cli_pwm_carrier_cmd},
	{"pwm_idle_test", "{idle_start|idle_stop}", cli_pwm_idle_test_cmd},
};

int cli_pwm_init(void)
{
	BK_LOG_ON_ERR(bk_pwm_driver_init());
	return cli_register_commands(s_pwm_commands, PWM_CMD_CNT);
}

#endif
