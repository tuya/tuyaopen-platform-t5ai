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

#if CONFIG_JPEG_SW_ENCODER_TEST
int jpeg_sw_enc_test(void);    
#endif

void cli_jpeg_sw_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_JPEG_SW_ENCODER_TEST
    int ret = 0;
    char *msg = NULL;

    if (argc == 1) 
    {
        ret = jpeg_sw_enc_test();
    } 
    else 
    {
        CLI_LOGE("usage: jpeg_sw_encoder_test\n");
        goto error;
    }

    if (!ret) {
        msg = WIFI_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }

error:
    msg = WIFI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
#endif
}

#define JPEG_SW_ENC_CMD_CNT (sizeof(s_jpeg_sw_enc_commands) / sizeof(struct cli_command))
static const struct cli_command s_jpeg_sw_enc_commands[] = {
    {"jpeg_sw_encoder_test", "jpeg_sw_encoder_test", cli_jpeg_sw_encoder_test_cmd},
};

int cli_jpeg_sw_enc_init(void)
{
    return cli_register_commands(s_jpeg_sw_enc_commands, JPEG_SW_ENC_CMD_CNT);
}

