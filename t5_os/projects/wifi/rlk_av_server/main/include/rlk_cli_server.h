#ifndef _BK_RLK_CLI_SERVER_H_
#define _BK_RLK_CLI_SERVER_H_
#ifdef __cplusplus
extern "C" {
#endif
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
#include "bk_cli.h"
#include "cli.h"
/**********************************************************/

#define RLK_TAG "rlks"

#define RLK_LOGI(...) BK_LOGI(RLK_TAG, ##__VA_ARGS__)
#define RLK_LOGW(...) BK_LOGW(RLK_TAG, ##__VA_ARGS__)
#define RLK_LOGE(...) BK_LOGE(RLK_TAG, ##__VA_ARGS__)
#define RLK_LOGD(...) BK_LOGD(RLK_TAG, ##__VA_ARGS__)


#define RLK_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define RLK_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

int rlk_cli_server_init(void);
#ifdef __cplusplus
}
#endif
#endif //_BK_RLK_CLI_SERVER_H_
// eof
