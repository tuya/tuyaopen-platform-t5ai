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
#include "os/os.h"
#include <driver/slcd_types.h>
#include <driver/slcd.h>


static void cli_slcd_display_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    slcd_config_t slcd_config;

    bk_slcd_set_seg_port_enable(0xFFFFFFF);
    slcd_config.com_num = SLCD_COM_NUM_4;
    slcd_config.slcd_bias = SLCD_BIAS_1_PER_OF_3;
    slcd_config.slcd_rate = SLCD_RATE_LEVEL_1;
    bk_slcd_driver_init(slcd_config);

    uint32_t demo_id = os_strtoul(argv[1], NULL, 10);
    if (demo_id == 1) {
        bk_slcd_set_seg00_03_value(0x0F0F0F0F);
        bk_slcd_set_seg04_07_value(0x0F0F0F0F);
        bk_slcd_set_seg08_11_value(0x0F0F0F0F);
        bk_slcd_set_seg12_15_value(0x0F0F0F0F);
        bk_slcd_set_seg16_19_value(0x0F0F0F0F);
        bk_slcd_set_com_port_enable(0xF);
        rtos_delay_milliseconds(3000);

        bk_slcd_set_com_port_enable(0x0);
        bk_slcd_set_seg00_03_value(0x0);
        bk_slcd_set_seg04_07_value(0x0);
        bk_slcd_set_seg08_11_value(0x0);
        bk_slcd_set_seg12_15_value(0x0);
        bk_slcd_set_seg16_19_value(0x0);

        while (1) {
            bk_slcd_set_seg00_03_value(0x01010101);
            bk_slcd_set_seg04_07_value(0x01010101);
            bk_slcd_set_seg08_11_value(0x01010101);
            bk_slcd_set_seg12_15_value(0x01010101);
            bk_slcd_set_seg16_19_value(0x01010101);
            bk_slcd_set_com_port_enable(0x1);
            rtos_delay_milliseconds(3000);
            bk_slcd_set_com_port_enable(0x0);
            bk_slcd_set_seg00_03_value(0x02020202);
            bk_slcd_set_seg04_07_value(0x02020202);
            bk_slcd_set_seg08_11_value(0x02020202);
            bk_slcd_set_seg12_15_value(0x02020202);
            bk_slcd_set_seg16_19_value(0x02020202);
            bk_slcd_set_com_port_enable(0x2);
            rtos_delay_milliseconds(3000);
            bk_slcd_set_com_port_enable(0x0);
            bk_slcd_set_seg00_03_value(0x04040404);
            bk_slcd_set_seg04_07_value(0x04040404);
            bk_slcd_set_seg08_11_value(0x04040404);
            bk_slcd_set_seg12_15_value(0x04040404);
            bk_slcd_set_seg16_19_value(0x04040404);
            bk_slcd_set_com_port_enable(0x4);
            rtos_delay_milliseconds(3000);
            bk_slcd_set_com_port_enable(0x0);
            bk_slcd_set_seg00_03_value(0x08080808);
            bk_slcd_set_seg04_07_value(0x08080808);
            bk_slcd_set_seg08_11_value(0x08080808);
            bk_slcd_set_seg12_15_value(0x08080808);
            bk_slcd_set_seg16_19_value(0x08080808);
            bk_slcd_set_com_port_enable(0x8);
            rtos_delay_milliseconds(3000);
        }
    } else if (demo_id == 2) {
        uint8_t i = 0;

        while (1) {
            for (i = SLCD_SEG_0; i < SLCD_SEG_19; i++) {
                bk_slcd_set_seg_value(i, 0x0F);
                bk_slcd_set_com_port_enable(0xF);
                rtos_delay_milliseconds(3000);
                bk_slcd_set_com_port_enable(0x0);
                bk_slcd_set_seg_value(i, 0x0);
            }
            rtos_delay_milliseconds(500);
        }
    }
}


#define SLCD_CMD_CNT    (sizeof(s_slcd_commands) / sizeof(struct cli_command))

static const struct cli_command s_slcd_commands[] = {
    {"slcd_test", "slcd_test {demo_id}", cli_slcd_display_test_cmd},
};

int cli_slcd_init(void)
{
    return cli_register_commands(s_slcd_commands, SLCD_CMD_CNT);
}


