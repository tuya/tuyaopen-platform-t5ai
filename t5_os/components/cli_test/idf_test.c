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
#include "bk_cli.h"
#include "at_common.h"

#define IDF_CMD_CNT (sizeof(s_idf_commands) / sizeof(struct cli_command))

const at_command_t *lookup_video_at_command(char *str1)
{

    for (int i = 0; i < video_at_cmd_cnt(); i++)
    {
        if (video_at_cmd_table[i].name == NULL) {
            i++;
            continue;
        }

        if(!os_strcmp(video_at_cmd_table[i].name, str1))
        {
            return &video_at_cmd_table[i];
        }
    }
    return NULL;
}

void videoat_command_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    const at_command_t *command = lookup_video_at_command(argv[1]);
    if (command == NULL) {
        bk_printf("cannot find this cmd, please check again!!!\n");
        msg = AT_CMD_RSP_ERROR;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }

    command->function(pcWriteBuffer, xWriteBufferLen, argc - 2, argv + 2);
}

static const struct cli_command s_idf_commands[] = {
    {"AT+VIDEO", "video cmd(open/read/close)", videoat_command_handler},
};

int cli_idf_init(void)
{
    return cli_register_commands(s_idf_commands, IDF_CMD_CNT);
}

