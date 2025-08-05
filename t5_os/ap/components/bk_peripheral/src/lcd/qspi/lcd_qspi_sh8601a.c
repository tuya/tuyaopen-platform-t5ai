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


#include <common/bk_include.h>
#include <driver/lcd_types.h>


#define LCD_QSPI_SHA8601A_REGISTER_WRITE_COMMAND        0x02
#define LCD_QSPI_SHA8601A_REGISTER_READ_COMMAND         0x03


static const lcd_qspi_init_cmd_t sh8601a_init_cmds[] =
{
	{0x11, {0x00}, 0},
	{0x00, {0x05}, 0xff},
	{0x2a, {0x00, 0x00, 0x01, 0xc5}, 4},
	{0x2b, {0x00, 0x00, 0x01, 0xc5}, 4},
#if CONFIG_SH8601A_PARTIAL
	{0x12, {0x00}, 0},//进入部分显示模式
#endif
	{0x44, {0x01, 0xc2}, 2},
	{0x35, {0x00}, 1},
	{0x53, {0x28}, 1},
	{0xc4, {0x84}, 1},
	{0x29, {0x00}, 0},
};

static uint8_t sh8601a_cmd[4] = {0x12, 0x00, 0x2c, 0x00};

static const lcd_qspi_t lcd_qspi_sh8601a_config =
{
	.clk = LCD_QSPI_80M,
	.refresh_method = LCD_QSPI_REFRESH_BY_FRAME,
	.reg_write_cmd = LCD_QSPI_SHA8601A_REGISTER_WRITE_COMMAND,
	.reg_read_cmd = LCD_QSPI_SHA8601A_REGISTER_READ_COMMAND,
	.reg_read_config.dummy_clk = 0,
	.reg_read_config.dummy_mode = LCD_QSPI_NO_INSERT_DUMMMY_CLK,
	.pixel_write_config.cmd = sh8601a_cmd,
	.pixel_write_config.cmd_len = sizeof(sh8601a_cmd),
	.init_cmd = sh8601a_init_cmds,
	.device_init_cmd_len = sizeof(sh8601a_init_cmds) / sizeof(lcd_qspi_init_cmd_t),
	.refresh_config = {0},
	.frame_len = (PPI_454X454 >> 16) * (PPI_454X454 & 0xFFFF) * 3,
};

const lcd_device_t lcd_device_sh8601a =
{
	.id = LCD_DEVICE_SH8601A,
	.name = "sh8601a",
	.type = LCD_TYPE_QSPI,
	.ppi = PPI_454X454,
	.qspi = &lcd_qspi_sh8601a_config,
	.init = NULL,
	.lcd_off = NULL,
};

