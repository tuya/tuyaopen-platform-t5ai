#pragma once
#include <stdlib.h>
#include <string.h>
#include <common/bk_include.h>
#include "cli_config.h"
#include <os/str.h>
#include <os/mem.h>
#include <os/os.h>
#include <common/bk_err.h>
#include <components/log.h>
#include <components/event.h>
#include <common/sys_config.h>
#include <driver/uart.h>
#include "bk_uart.h"
#include <driver/uart.h>
#include "bk_cli.h"
#include "cli.h"

#ifdef __cplusplus
extern "C" {
#endif



extern int wdrv_cli_init(void);
void a35_demo_connect(char *oob_ssid, char *connect_key);

#ifdef __cplusplus
}
#endif



