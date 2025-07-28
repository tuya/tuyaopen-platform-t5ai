// Copyright 2020-2024 Beken
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
#if CONFIG_SADC_API_TEST
#include <os/os.h>
#include "cli.h"
#include "argtable3.h"
#include "cli_common.h"
#include "adc_cmd_api.h"

//void bk_dump_hex(const char *prefix, const void *ptr, uint32_t buflen);

static void cli_sadc_driver_init_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        BK_LOG_ON_ERR(bk_adc_driver_init_test());
        CLI_LOGI("sadc driver init over\n");
}

static void cli_sadc_driver_deinit_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        BK_LOG_ON_ERR(bk_adc_driver_deinit_test());
        CLI_LOGI("sadc driver deinit over\n");
}


#define CONFIG_STRUCT_FIELD_CNT   10
#define ARG_ERR_REC_CNT           20

static void _sadc_channel_init_cmd_handler(void **argtable)
{
        adc_config_t adc_config = {0};
        struct arg_lit *help = (struct arg_lit *)argtable[0];
        struct arg_lit *arg_config = (struct arg_lit *)argtable[1];
        struct arg_int *arg_config_detail = (struct arg_int *)argtable[2];
        
        if(help->count > 0)
        {
            print_help(argtable[0], argtable);
            return;
        }

        if((0 < arg_config->count) && (arg_config_detail->count >= CONFIG_STRUCT_FIELD_CNT))
        {
                adc_config.clk = (arg_config_detail->ival[0]);
                adc_config.sample_rate = (arg_config_detail->ival[1]);
                adc_config.adc_filter = (arg_config_detail->ival[2]);
                adc_config.steady_ctrl = (arg_config_detail->ival[3]);
                adc_config.is_hw_using_cali_result = (arg_config_detail->ival[4]);
                adc_config.adc_mode = (arg_config_detail->ival[5]);
                adc_config.src_clk = (arg_config_detail->ival[6]);
                adc_config.chan = (arg_config_detail->ival[7]);
                adc_config.saturate_mode = (arg_config_detail->ival[8]);
                adc_config.vol_div = (arg_config_detail->ival[9]);

                CLI_LOGI("adc_config.clk:0x%x\n", adc_config.clk);
                CLI_LOGI("adc_config.sample_interval:0x%x\n", adc_config.sample_rate);
                CLI_LOGI("adc_config.adc_filter:0x%x\n", adc_config.adc_filter);
                CLI_LOGI("adc_config.steady_ctrl:0x%x\n", adc_config.steady_ctrl);
                CLI_LOGI("adc_config.is_hw_using_cali_result:0x%x\n", adc_config.is_hw_using_cali_result);
                CLI_LOGI("adc_config.adc_mode:0x%x\n", adc_config.adc_mode);
                CLI_LOGI("adc_config.src_clk:0x%x\n", adc_config.src_clk);
                CLI_LOGI("adc_config.chan:0x%x\n", adc_config.chan);
                CLI_LOGI("adc_config.saturate_mode:0x%x\n", adc_config.saturate_mode);
                CLI_LOGI("adc_config.vol_div:0x%x\n", adc_config.vol_div);

                BK_LOG_ON_ERR(bk_adc_channel_init(&adc_config));
                CLI_LOGI("sadc channel init over\n");
        }
}

static void cli_sadc_channel_init_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
        struct arg_lit *arg_config = arg_lit0("c", "config", "config");
        struct arg_int *arg_config_detail = arg_intn(NULL, NULL, "<clk/sample_interval/adc_filter/steady_ctrl/is_using_cali/adc_mode/src_clk/channel_id/saturate_mode/vol_div>", CONFIG_STRUCT_FIELD_CNT, CONFIG_STRUCT_FIELD_CNT, "configure the specified channel");
        struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

        void* argtable[] = { help, arg_config, arg_config_detail, end };
        int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

        common_cmd_handler(argc, argv, argtable, argtable_size, _sadc_channel_init_cmd_handler);
}

static void _sadc_channel_deinit_cmd_handler(void **argtable)
{
        uint8_t channel_id;
        struct arg_lit *help = (struct arg_lit *)argtable[0];
        struct arg_int *arg_channel = (struct arg_int *)argtable[1];
        
        if(help->count > 0)
        {
            print_help(argtable[0], argtable);
            return;
        }

        if(arg_channel->count > 0)
        {
                channel_id = arg_channel->ival[0];
                BK_LOG_ON_ERR(bk_adc_channel_deinit(channel_id));
                CLI_LOGI("sadc channel deinit over\n");
        }
}

static void cli_sadc_channel_deinit_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
        struct arg_int *arg_channel = arg_int1("c", "channel", "<id>", "deinit the specified channel");
        struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

        void* argtable[] = { help, arg_channel, end };
        int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

        common_cmd_handler(argc, argv, argtable, argtable_size, _sadc_channel_deinit_cmd_handler);
}

static void _sadc_channel_read_cmd_handler(void **argtable)
{
        uint8_t channel_id;
        uint16_t *buffer;
        uint32_t timeout;
        struct arg_lit *help = (struct arg_lit *)argtable[0];
        struct arg_int *arg_channel = (struct arg_int *)argtable[1];
        struct arg_int *arg_buf = (struct arg_int *)argtable[2];
        struct arg_int *arg_timeout = (struct arg_int *)argtable[3];
        
        if(help->count > 0)
        {
            print_help(argtable[0], argtable);
            return;
        }

        if((arg_channel->count > 0) && (arg_buf->count > 0) && (arg_timeout->count > 0))
        {
                channel_id = arg_channel->ival[0];
                buffer = (uint16_t *)arg_buf->ival[0];
                timeout = arg_timeout->ival[0];

                BK_LOG_ON_ERR(bk_adc_channel_read(channel_id, buffer, timeout));
                CLI_LOGI("sadc channel read:0x%04x over\n", *buffer);
        }
}

static void cli_sadc_channel_read_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
        struct arg_int *arg_channel = arg_int1("c", "channel", "<id>", "select the specified channel");
        struct arg_int *arg_sample_buf = arg_int1("b", "buffer", "<buffer_pointer>", "one-sample buffer space");
        struct arg_int *arg_timeout = arg_int1("t", "timeout", "<timeout_duration>", "timeout period of getting one sample");
        struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

        void* argtable[] = { help, arg_channel, arg_sample_buf, arg_timeout, end};
        int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

        common_cmd_handler(argc, argv, argtable, argtable_size, _sadc_channel_read_cmd_handler);
}

static void _sadc_channel_raw_read_cmd_handler(void **argtable)
{
        uint8_t channel_id;
        uint16_t *buffer;
        uint32_t sample_cnt, timeout;
        struct arg_lit *help = (struct arg_lit *)argtable[0];
        struct arg_int *arg_channel = (struct arg_int *)argtable[1];
        struct arg_int *arg_buf = (struct arg_int *)argtable[2];
        struct arg_int *arg_sample_cnt = (struct arg_int *)argtable[3];
        struct arg_int *arg_timeout = (struct arg_int *)argtable[4];
        
        if(help->count > 0)
        {
            print_help(argtable[0], argtable);
            return;
        }

        if((arg_channel->count > 0) && (arg_buf->count > 0) 
                && (arg_timeout->count > 0) && (arg_sample_cnt->count > 0))
        {
                channel_id = arg_channel->ival[0];
                buffer = (uint16_t *)arg_buf->ival[0];
                timeout = arg_timeout->ival[0];
                sample_cnt = arg_sample_cnt->ival[0];


                BK_LOG_ON_ERR(bk_adc_channel_raw_read(channel_id, buffer, sample_cnt, timeout));

                //bk_dump_hex("samples:", buffer, sample_cnt * 2);
                for(UINT32 i = 0; i < sample_cnt; i++)
                {
                    CLI_LOGI("samples=%d\r\n",buffer[i]);
                }
                CLI_LOGI("sadc channel raw read over\n");
        }
}

static void cli_sadc_channel_raw_read_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
        struct arg_int *arg_channel = arg_int1("c", "channel", "<id>", "select the specified channel");
        struct arg_int *arg_samples_buf = arg_int1("b", "buffer", "<buffer_pointer>", "samples buffer space");
        struct arg_int *arg_samples_cnt = arg_int1("n", "count", "<value>", "sample count");
        struct arg_int *arg_timeout = arg_int1("t", "timeout", "<timeout_duration>", "timeout period of getting one sample");
        struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

        void* argtable[] = { help, arg_channel, arg_samples_buf, arg_samples_cnt, arg_timeout, end};
        int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

        common_cmd_handler(argc, argv, argtable, argtable_size, _sadc_channel_raw_read_cmd_handler);
}

static void _sadc_register_isr_callback_cmd_handler(void **argtable)
{
        adc_isr_t isr;
        uint32_t param;
        struct arg_lit *help = (struct arg_lit *)argtable[0];
        struct arg_int *arg_cb = (struct arg_int *)argtable[1];
        struct arg_int *arg_param = (struct arg_int *)argtable[2];
        
        if(help->count > 0)
        {
            print_help(argtable[0], argtable);
            return;
        }

        if((arg_cb->count > 0) && (arg_param->count > 0))
        {
                isr = (adc_isr_t)arg_cb->ival[0];
                param = arg_param->ival[0];

                BK_LOG_ON_ERR(bk_adc_register_isr_callback(isr, param));
                CLI_LOGI("sadc register isr callback over\n");
        }
}

static void cli_sadc_isr_cb_register_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
        struct arg_int *arg_cb = arg_int1("c", "callback", "<function_pointer>", "callback function pointer");
        struct arg_int *arg_param = arg_int1("p", "param", "<function_param>", "parameter of callback function");
        struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

        void *argtable[] = { help, arg_cb, arg_param, end};
        int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

        common_cmd_handler(argc, argv, argtable, argtable_size, _sadc_register_isr_callback_cmd_handler);
}

static void cli_sadc_isr_cb_unregister_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        BK_LOG_ON_ERR(bk_adc_unregister_isr_callback());
        CLI_LOGI("sadc isr callback unregister over\n");
}

#define CLI_SECTION(x) __attribute__((used, section(x)))
#define CLI_CMD_LABLE  CLI_SECTION(".cli_cmdtabl")
#define DRV_CLI_CMD_EXPORT CLI_CMD_LABLE

#define SADC_CMD_CNT (sizeof(s_sadc_api_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_sadc_api_commands[] = {
    {"sadc_drv_init", "sadc_drv_init", cli_sadc_driver_init_cmd},
    {"sadc_drv_deinit", "sadc_drv_deinit", cli_sadc_driver_deinit_cmd},
    {"sadc_chan_init", "sadc_chan_init {config}", cli_sadc_channel_init_cmd},
    {"sadc_chan_deinit", "sadc_chan_deinit {id}", cli_sadc_channel_deinit_cmd},
    {"sadc_read", "sadc_read {chan|buf|timeout}", cli_sadc_channel_read_cmd},
    {"sadc_raw_read", "sadc_raw_read {chan|buf|sample_cnt|timeout}", cli_sadc_channel_raw_read_cmd},
    {"sadc_isr_cb_reg", "sadc_isr_cb_reg {cb|param}", cli_sadc_isr_cb_register_cmd},
    {"sadc_isr_cb_unreg", "sadc_isr_cb_unreg", cli_sadc_isr_cb_unregister_cmd},
};
#endif

int cli_sadc_register_cli_api_test_feature(void)
{
#if CONFIG_SADC_API_TEST
    return cli_register_commands(s_sadc_api_commands, SADC_CMD_CNT);
#else
    return 0;
#endif
}
// eof
