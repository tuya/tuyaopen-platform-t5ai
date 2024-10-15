#ifndef __RLK_PS_CLIENT_H__
#define __RLK_PS_CLIENT_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_typedef.h>


void rlk_client_rtc_sleep_start(UINT8 pm_sleep_mode, UINT32 rtc_time_ms, UINT8 pm_mode_name1,UINT8 pm_mode_name2,UINT8 pm_mode_name3,UINT8 pm_param);

#ifdef __cplusplus
}
#endif
#endif //__RLK_PS_CLIENT_H__
// eof

