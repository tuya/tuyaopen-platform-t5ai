#ifndef _BK_PHY_IPC_H_
#define _BK_PHY_IPC_H_

#include <common/bk_include.h>

enum
{
	PHY_CMD_GET_TEMP,
	PHY_CMD_GET_VOLT,
};

typedef struct
{
    float                        param;
    u32                          timeout;
    int16                        ret_status;
    u32                          crc;
} phy_cmd_t;

#define PHY_IPC_READ_SIZE     0x400
#define PHY_IPC_WRITE_SIZE    0x400

#endif //_SARADC_IPC_H_
// eof