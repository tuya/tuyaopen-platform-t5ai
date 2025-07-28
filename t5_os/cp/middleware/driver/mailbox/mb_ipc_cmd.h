// Copyright 2020-2022 Beken
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

#ifndef _mb_ipc_cmd_h_
#define _mb_ipc_cmd_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_typedef.h>
#include <common/bk_err.h>

enum
{
	IPC_TEST_CMD = 0,               /* Server <-> Client */

	IPC_CPU1_POWER_UP_INDICATION,           /* CPU1 -> CPU0  */
	IPC_CPU1_HEART_BEAT_INDICATION,         /* CPU1 -> CPU0  */
	IPC_GET_CPU1_HEART_RATE,                /* CPU0 -> CPU1  */
	IPC_SET_CPU1_HEART_RATE,                /* CPU0 -> CPU1  */

	IPC_GET_POWER_SAVE_FLAG,                /* Server -> Client  */

	IPC_RES_ACQUIRE_CNT,                    /* Client -> Server  */
	IPC_RES_RELEASE_CNT,                    /* Client -> Server  */

	IPC_RES_AVAILABLE_INDICATION,   /* Server <-> Client */
	IPC_ALLOC_DMA_CHNL,                     /* Client -> Server  */
	IPC_FREE_DMA_CHNL,                      /* Client -> Server  */
	IPC_DMA_CHNL_USER,                      /* Client -> Server  */

	IPC_CPU1_TRAP_HANDLE_BEGIN,             /* CPU1 -> CPU0  */
	IPC_CPU1_TRAP_HANDLE_END,               /* CPU1 -> CPU0  */
	IPC_CPU1_NEED_REBOOT,                   /* CPU1 -> CPU0  */

	IPC_CPU0_START_USB_CDC,
	IPC_CPU0_OPEN_USB_CDC,
	IPC_CPU0_CLOSE_USB_CDC,
	IPC_CPU0_STOP_USB_CDC,

	IPC_CPU0_SET_USB_CDC_CMD,
	IPC_CPU0_GET_USB_CDC_DATA,
	IPC_CPU0_INIT_USB_CDC_PARAM,

	IPC_CPU1_UPDATE_USB_CDC_STATE,
	IPC_CPU1_UPLOAD_USB_CDC_DATA,

	IPC_CMD_MAX  = 0x7F,  /* cmd id can NOT great than 0x7F. */
};

#if (CONFIG_CPU_CNT > 1)

#define AMP_RES_SERVER


#include "amp_res_lock.h"

bk_err_t ipc_init(void);

u32      ipc_send_test_cmd(u32 param);
bk_err_t ipc_send_available_ind(u16 resource_id);
u32      ipc_send_get_ps_flag(void);

u32      ipc_send_get_heart_rate(void);
bk_err_t ipc_send_set_heart_rate(u32 param);


#ifdef AMP_RES_CLIENT
bk_err_t ipc_send_res_acquire_cnt(u16 resource_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list);
bk_err_t ipc_send_res_release_cnt(u16 resource_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list);
u8       ipc_send_alloc_dma_chnl(u32 user_id);
bk_err_t ipc_send_free_dma_chnl(u32 user_id, u8 chnl_id);
u32      ipc_send_dma_chnl_user(u8 chnl_id);
bk_err_t ipc_send_cpu1_need_reboot(void);
#endif

#else  /* (CONFIG_CPU_CNT > 1) */

#define ipc_init(void)	

#endif  /* (CONFIG_CPU_CNT > 1) */

/**    ============================    RPC end   ============================   **/

#ifdef __cplusplus
}
#endif

#endif /* _mb_ipc_cmd_h_ */

