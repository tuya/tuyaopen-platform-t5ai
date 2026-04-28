/**
* Copyright (C) by Tuya Inc
* All rights reserved
*
* @file tkl_mftest.h
*/

#ifndef __TKL_MFTEST_H__
#define __TKL_MFTEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

typedef enum {
    TKL_AUDIO_TEST_START_CMD = 0x0001,
    TKL_AUDIO_TEST_STOP_CMD  = 0x0002,
    TKL_USB_INFO_TEST_CMD    = 0x0003
} TKL_MFTEST_CMD_E;

OPERATE_RET tkl_mftest_ctrl(uint16_t cmd, uint8_t *in, uint32_t inlen, uint8_t **out, uint16_t *outlen);


#ifdef __cplusplus
}
#endif

#endif /* __TKL_MFTEST_H__ */
