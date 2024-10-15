#ifndef _BK_RLK_CO_H_
#define _BK_RLK_CO_H_

#include <stdlib.h>
#include <string.h>

#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>

#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/raw_link.h>
#include "rlk_demo_cli.h"
#include "rlk_ping.h"
#include <modules/raw_link.h>

#define RLK_INVALID_INDEX     0xff

/*RLK APP TYPE*/
#define RLK_APP_TYPE_DATA 0
#define RLK_APP_TYPE_MGMT 1
/*RLK APP SUBTYPE*/
#define RLK_APP_NORMAL_DATA 0
#define RLK_APP_ECHO_REQ    1
#define RLK_APP_ECHO_RPLY   2

typedef struct
{
    uint8_t  type:2;
    uint8_t  subtype:6;
    uint8_t  len;
    uint16_t seq;
}RLK_APP_HDR;

typedef struct
{
    uint8_t is_inited;
    uint8_t chnl;
}RLK_DEMO;

extern int rlk_param_find_id(int argc, char **argv, char *param);
extern int rlk_param_find(int argc, char **argv, char *param);
extern uint32_t rlk_sys_now(void);
bk_err_t rlk_demo_dispatch(bk_rlk_recv_info_t *rx_info);

extern RLK_DEMO rlk_demo_env;

#endif /*end of _BK_RLK_CO_H_*/
