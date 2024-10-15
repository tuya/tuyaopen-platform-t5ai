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

#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#if GAP_IS_OLD_API
    #include "components/bluetooth/bk_dm_ble.h"
#else
    #include "components/bluetooth/bk_dm_bluetooth_types.h"
    #include "components/bluetooth/bk_dm_gap_ble_types.h"
    #include "components/bluetooth/bk_dm_gap_ble.h"
#endif

#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_ble_types.h"
#include "dm_gatts.h"
#include "dm_gatt_connection.h"

#define DYNAMIC_ADD_ATTR 0


#define INVALID_ATTR_HANDLE 0
#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))


//typedef struct
//{
//    uint8_t notify_status; //0 disable; 1 notify; 2 indicate
//} dm_gatts_app_env_t;

#define dm_gatts_app_env_t dm_gatt_addition_app_env_t

static beken_semaphore_t s_ble_sema = NULL;
static uint16_t s_service_attr_handle = INVALID_ATTR_HANDLE;

static uint16_t s_char_attr_handle = INVALID_ATTR_HANDLE;
static uint8_t s_char_buff[4] = {0};

static uint16_t s_char_desc_attr_handle = INVALID_ATTR_HANDLE;
static uint16_t s_char_desc_buff = 0;

static uint16_t s_char2_attr_handle = INVALID_ATTR_HANDLE;
static uint8_t s_char2_buff[312] = {0};

static uint16_t s_char2_desc_attr_handle = INVALID_ATTR_HANDLE;
static uint32_t s_char2_desc_buff = 0;

static uint16_t s_char_auto_rsp_attr_handle = INVALID_ATTR_HANDLE;
static uint8_t s_char_auto_rsp_buff[300] = {0};

static uint16_t s_char4_attr_handle = INVALID_ATTR_HANDLE;

static beken_timer_t s_char_notify_timer;
static uint16_t s_conn_id = 0xff;

static bk_gatt_if_t s_gatts_if;
//static uint8_t s_timer_send_is_indicate = 0;

#if DYNAMIC_ADD_ATTR

#else

#define BK_GATT_ATTR_TYPE(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}
#define BK_GATT_ATTR_CONTENT(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}
#define BK_GATT_ATTR_VALUE(ilen, ivalue) {.attr_max_len = ilen, .attr_len = ilen, .attr_value = ivalue}


#define BK_GATT_PRIMARY_SERVICE_DECL(iuuid) \
    .att_desc =\
               {\
                .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_PRI_SERVICE),\
                .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
               }

#define BK_GATT_CHAR_DECL(iuuid, ilen, ivalue, iprop, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_CHAR_DECLARE),\
                 .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .prop = iprop,\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

#define BK_GATT_CHAR_DESC_DECL(iuuid, ilen, ivalue, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

static const bk_gatts_attr_db_t s_gatts_attr_db_service_1[] =
{
    //service
    {
        BK_GATT_PRIMARY_SERVICE_DECL(0x1234),
    },

    //char 1
    {
        BK_GATT_CHAR_DECL(0x5678,
                          sizeof(s_char_buff), s_char_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          //BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          //BK_GATT_PERM_READ_ENC_MITM | BK_GATT_PERM_WRITE_ENC_MITM, //gap iocap must not be BK_IO_CAP_NONE !!!
                          BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_char_desc_buff), (uint8_t *)&s_char_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },

    //char 2
    {
        BK_GATT_CHAR_DECL(0x9abc,
                          sizeof(s_char2_buff), s_char2_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          //BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          //BK_GATT_PERM_READ_ENC_MITM | BK_GATT_PERM_WRITE_ENC_MITM, //gap iocap must not be BK_IO_CAP_NONE !!!
                          BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_SRVR_CONFIG,
                               sizeof(s_char2_desc_buff), (uint8_t *)&s_char2_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },

    //char 3
    {
        BK_GATT_CHAR_DECL(0x15ab,
                          sizeof(s_char_auto_rsp_buff), s_char_auto_rsp_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },

    //char 4 no buffer
    {
        BK_GATT_CHAR_DECL(0x15ac,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP), //Without buffer, BK_GATT_AUTO_RSP will cause "NOT PERMITTED" err to peer when recv write or read req. So set BK_GATT_RSP_BY_APP instead.
    },
};


static const bk_gatts_attr_db_t s_gatts_attr_db_service_2[] =
{
    //service
    {
        BK_GATT_PRIMARY_SERVICE_DECL(0x2234),
    },

    //char 1
    {
        BK_GATT_CHAR_DECL(0x5678,
                          sizeof(s_char_buff), s_char_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          //BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          //BK_GATT_PERM_READ_ENC_MITM | BK_GATT_PERM_WRITE_ENC_MITM, //gap iocap must not be BK_IO_CAP_NONE !!!
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_char_desc_buff), (uint8_t *)&s_char_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },

    //char 2
    {
        BK_GATT_CHAR_DECL(0x9abc,
                          sizeof(s_char2_buff), s_char2_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_SRVR_CONFIG,
                               sizeof(s_char2_desc_buff), (uint8_t *)&s_char2_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },

    //char 3
    {
        BK_GATT_CHAR_DECL(0x15ab,
                          sizeof(s_char_auto_rsp_buff), s_char_auto_rsp_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ_ENC_MITM | BK_GATT_PERM_WRITE_ENC_MITM,
                          BK_GATT_AUTO_RSP),
    },
};



static uint16_t *const s_attr_handle_list[sizeof(s_gatts_attr_db_service_1) / sizeof(s_gatts_attr_db_service_1[0])] =
{
    &s_service_attr_handle,
    &s_char_attr_handle,
    &s_char_desc_attr_handle,
    &s_char2_attr_handle,
    &s_char2_desc_attr_handle,
    &s_char_auto_rsp_attr_handle,
    &s_char4_attr_handle,
};


static uint16_t s_attr_handle_list2[sizeof(s_gatts_attr_db_service_2) / sizeof(s_gatts_attr_db_service_2[0])];


#endif

#if GAP_IS_OLD_API

static void dm_ble_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    param->status;

    switch (cmd)
    {
    case BLE_CREATE_ADV:
    case BLE_SET_ADV_DATA:
    case BLE_SET_RSP_DATA:
    case BLE_START_ADV:
    case BLE_STOP_ADV:
    case BLE_SET_RANDOM_ADDR:
        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }

        break;

    default:
        break;
    }

}

#else

static int32_t dm_ble_gap_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
    gatt_logd("event %d", event);

    switch (event)
    {
    case BK_BLE_GAP_SET_STATIC_RAND_ADDR_EVT:
    {
        struct ble_set_rand_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set rand addr err %d", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
    {
        struct ble_adv_set_rand_addr_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set adv rand addr err %d", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT:
    {
        struct ble_adv_params_set_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set adv param err 0x%x", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT:
    {
        struct ble_adv_data_set_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set adv data err %d", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    {
        struct ble_adv_scan_rsp_set_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set adv data scan rsp err %d", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT:
    {
        struct ble_adv_start_cmpl_evt_param *pm = (typeof(pm))param;

        if (pm->status)
        {
            gatt_loge("set adv enable err %d", pm->status);
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    default:
        return DM_BLE_GAP_APP_CB_RET_NO_INTERESTING;
        break;

    }

    return DM_BLE_GAP_APP_CB_RET_PROCESSED;
}

#endif

static int32_t nest_func_send_indicate(dm_gatt_app_env_t *env, void *arg)
{
    uint8_t *tmp_buff = (typeof(tmp_buff))arg;

    if (env && env->status == GAP_CONNECT_STATUS_CONNECTED && env->conn_id != 0xffff && env->data) //env->local_is_master == 0
    {
        dm_gatts_app_env_t *tmp = (typeof(tmp))env->data;

        if (tmp->notify_status)
        {
            if (bk_ble_gatts_send_indicate(0, env->conn_id, s_char_attr_handle, sizeof(s_char_buff), tmp_buff, tmp->notify_status == 2 ? 1 : 0))
            {
                gatt_loge("notify err");
            }
        }
    }

    return 0;
}

static int32_t nest_func_check_timer_ref_count(dm_gatt_app_env_t *env, void *arg)
{
    uint8_t *tmp_count = arg;

    if (env && env->status == GAP_CONNECT_STATUS_CONNECTED && env->conn_id != 0xffff && env->data) //&& env->local_is_master == 0
    {
        dm_gatts_app_env_t *tmp_env = (typeof(tmp_env))env->data;

        if (tmp_env->notify_status)
        {
            (*tmp_count)++;
        }
    }

    return 0;
}

static void ble_char_timer_callback(void *param)
{
    static uint8_t value = 1;

    uint8_t *tmp_buff = NULL;
    uint32_t type = (typeof(type))param;

    if (s_conn_id == 0xff)
    {
        return;
    }

    tmp_buff = os_malloc(sizeof(s_char_buff));

    if (!tmp_buff)
    {
        gatt_loge("alloc send failed");
        return;
    }

    os_memset(tmp_buff, 0, sizeof(s_char_buff));
    tmp_buff[0] = value++;

    //nest func only for gcc

    dm_ble_app_env_foreach(nest_func_send_indicate, tmp_buff);
    //retval = bk_ble_gatts_send_indicate(0, s_conn_id, s_char_attr_handle, sizeof(s_char_buff), tmp_buff, s_timer_send_is_indicate);

    os_free(tmp_buff);

    //    if (retval != 0)
    //    {
    //        gatt_loge("notify err %d", retval);
    //    }
}

static int32_t bk_gatts_cb (bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    dm_gatt_app_env_t *common_env_tmp = NULL;
    dm_gatts_app_env_t *app_env_tmp = NULL;

    switch (event)
    {
    case BK_GATTS_REG_EVT:
    {
        struct gatts_reg_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_REG_EVT %d %d", param->status, param->gatt_if);
        s_gatts_if = param->gatt_if;

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_START_EVT:
    {
        struct gatts_start_evt_param *param = (typeof(param))comm_param;
        gatt_logi("BK_GATTS_START_EVT compl %d %d", param->status, param->service_handle);

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_STOP_EVT:
    {
        struct gatts_stop_evt_param *param = (typeof(param))comm_param;
        gatt_logi("BK_GATTS_STOP_EVT compl %d %d", param->status, param->service_handle);

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_CREAT_ATTR_TAB_EVT:
    {
#if DYNAMIC_ADD_ATTR
#else
        struct gatts_add_attr_tab_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_CREAT_ATTR_TAB_EVT %d %d", param->status, param->num_handle);

        if (s_service_attr_handle == INVALID_ATTR_HANDLE)
        {
            for (int i = 0; i < param->num_handle; ++i)
            {
                *s_attr_handle_list[i] = param->handles[i];

                if (i) //service handle cant get buff
                {
                    uint16_t tmp_len = 0;
                    uint8_t *tmp_buff = NULL;
                    ble_err_t g_status = 0;

                    g_status = bk_ble_gatts_get_attr_value(param->handles[i], &tmp_len, &tmp_buff);

                    if (g_status)
                    {
                        gatt_loge("get attr value err %d", g_status);
                    }


                    if (tmp_len != s_gatts_attr_db_service_1[i].att_desc.value.attr_len ||
                            tmp_buff != s_gatts_attr_db_service_1[i].att_desc.value.attr_value)
                    {
                        gatt_loge("get attr value not match create attr handle %d i %d %d %d %p %p!!!!",
                                  param->handles[i], i,
                                  tmp_len, s_gatts_attr_db_service_1[i].att_desc.value.attr_len,
                                  tmp_buff, s_gatts_attr_db_service_1[i].att_desc.value.attr_value);
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < param->num_handle; ++i)
            {
                s_attr_handle_list2[i] = param->handles[i];

                if (i) //service handle cant get buff
                {
                    uint16_t tmp_len = 0;
                    uint8_t *tmp_buff = NULL;
                    ble_err_t g_status = 0;

                    g_status = bk_ble_gatts_get_attr_value(param->handles[i], &tmp_len, &tmp_buff);

                    if (g_status)
                    {
                        gatt_loge("get attr value err %d", g_status);
                    }


                    if (tmp_len != s_gatts_attr_db_service_2[i].att_desc.value.attr_len ||
                            tmp_buff != s_gatts_attr_db_service_2[i].att_desc.value.attr_value)
                    {
                        gatt_loge("get attr value not match create attr handle %d i %d %d %d %p %p!!!!",
                                  param->handles[i], i,
                                  tmp_len, s_gatts_attr_db_service_2[i].att_desc.value.attr_len,
                                  tmp_buff, s_gatts_attr_db_service_2[i].att_desc.value.attr_value);
                    }
                }
            }
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }

#endif
    }
    break;

    case BK_GATTS_CREATE_EVT:
    {
        struct gatts_create_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_CREATE_EVT %d %d", param->status, param->service_handle);
        s_service_attr_handle = param->service_handle;

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_ADD_CHAR_EVT:
    {
        struct gatts_add_char_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_ADD_CHAR_EVT %d %d %d", param->status, param->attr_handle, param->service_handle);

        if (s_char_attr_handle == INVALID_ATTR_HANDLE)
        {
            s_char_attr_handle = param->attr_handle;
        }
        else if (s_char2_attr_handle == INVALID_ATTR_HANDLE)
        {
            s_char2_attr_handle = param->attr_handle;
        }
        else
        {
            s_char_auto_rsp_attr_handle = param->attr_handle;
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_ADD_CHAR_DESCR_EVT:
    {
        struct gatts_add_char_descr_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_ADD_CHAR_DESCR_EVT %d %d %d", param->status, param->attr_handle, param->service_handle);

        if (s_char_desc_attr_handle == INVALID_ATTR_HANDLE)
        {
            s_char_desc_attr_handle = param->attr_handle;
        }
        else if (s_char2_desc_attr_handle == INVALID_ATTR_HANDLE)
        {
            s_char2_desc_attr_handle = param->attr_handle;
        }

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }
    break;

    case BK_GATTS_READ_EVT:
    {
        struct gatts_read_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        gatt_logi("BK_GATTS_READ_EVT %d %d %d %d %d %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id, param->trans_id, param->handle,
                  param->offset, param->is_long, param->need_rsp,
                  param->bda[5],
                  param->bda[4],
                  param->bda[3],
                  param->bda[2],
                  param->bda[1],
                  param->bda[0]);

        memset(&rsp, 0, sizeof(rsp));

        if (param->handle == s_char_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = sizeof(s_char_buff) - param->offset;

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = s_char_buff + param->offset;
                rsp.attr_value.len = final_len;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else if (param->handle == s_char_desc_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = sizeof(s_char_desc_buff) - param->offset;

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = (uint8_t *)&s_char_desc_buff + param->offset;
                rsp.attr_value.len = final_len;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }

        }
        else if (param->handle == s_char2_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = sizeof(s_char2_buff) - param->offset;

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = s_char2_buff + param->offset;
                rsp.attr_value.len = final_len;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else if (param->handle == s_char2_desc_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = sizeof(s_char2_desc_buff) - param->offset;

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = (uint8_t *)&s_char2_desc_buff + param->offset;
                rsp.attr_value.len = final_len;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
                //ret = bk_ble_gatts_send_response(s_gatt_if, param->conn_id, param->trans_id, BK_GATT_INSUF_RESOURCE, &rsp);
            }
        }
        else if (param->handle == s_char_auto_rsp_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = sizeof(s_char_auto_rsp_buff) - param->offset;

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = s_char_auto_rsp_buff + param->offset;
                rsp.attr_value.len = final_len;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else
        {
            if (param->need_rsp)
            {
                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_REQ_NOT_SUPPORTED, &rsp);
            }
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        memset(&rsp, 0, sizeof(rsp));

        gatt_logi("BK_GATTS_WRITE_EVT %d %d %d %d %d %d %d 0x%02X%02X %02X:%02X:%02X:%02X:%02X:%02X",
                  param->conn_id, param->trans_id, param->handle,
                  param->offset, param->need_rsp, param->is_prep, param->len,
                  param->value[0], param->value[1],
                  param->bda[5],
                  param->bda[4],
                  param->bda[3],
                  param->bda[2],
                  param->bda[1],
                  param->bda[0]);

        if (param->handle == s_char_attr_handle)
        {
            gatt_logi("len %d 0x%02X%02X%02X%02X", param->len,
                      param->value[0],
                      param->value[1],
                      param->value[2],
                      param->value[3]);

            if (param->need_rsp)
            {
                final_len = MIN_VALUE(param->len, sizeof(s_char_buff) - param->offset);
                memcpy(s_char_buff + param->offset, param->value, final_len);

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = s_char_buff + param->offset;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else if (param->handle == s_char_desc_attr_handle)
        {
            uint16_t config = (((uint16_t)(param->value[1])) << 8) | param->value[0];

            common_env_tmp = dm_ble_find_app_env_by_addr(param->bda);

            if (!common_env_tmp || !common_env_tmp->data)
            {
                gatt_loge("conn max %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp : 0);
                break;
            }

            app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

            if (config & 1)
            {
                gatt_logi("client notify open");

                app_env_tmp->notify_status = 1;

                if (!rtos_is_timer_init(&s_char_notify_timer))
                {
                    rtos_init_timer(&s_char_notify_timer, 1000, ble_char_timer_callback, (void *)0);
                    rtos_start_timer(&s_char_notify_timer);
                }
            }
            else if (config & 2)
            {
                gatt_logi("client indicate open");

                app_env_tmp->notify_status = 2;

                if (!rtos_is_timer_init(&s_char_notify_timer))
                {
                    rtos_init_timer(&s_char_notify_timer, 1000, ble_char_timer_callback, (void *)0);
                    rtos_start_timer(&s_char_notify_timer);
                }
            }
            else if (!config)
            {
                uint8_t timer_ref_count = 0;
                gatt_logi("client config close");

                app_env_tmp->notify_status = 0;

                //nest func only for gcc

                dm_ble_app_env_foreach(nest_func_check_timer_ref_count, &timer_ref_count);

                if (!timer_ref_count && rtos_is_timer_init(&s_char_notify_timer))
                {
                    if (rtos_is_timer_running(&s_char_notify_timer))
                    {
                        rtos_stop_timer(&s_char_notify_timer);
                    }

                    rtos_deinit_timer(&s_char_notify_timer);
                }
            }
            else
            {
                gatt_logi("client config close");
            }

            if (param->need_rsp)
            {
                final_len = MIN_VALUE(param->len, sizeof(s_char_desc_attr_handle) - param->offset);
                memcpy((uint8_t *)&s_char_desc_buff + param->offset, param->value, final_len);

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = (uint8_t *)&s_char_desc_buff + param->offset;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else if (param->handle == s_char2_attr_handle)
        {
            if (param->need_rsp)
            {
                final_len = MIN_VALUE(param->len, sizeof(s_char2_buff) - param->offset);
                memcpy(s_char2_buff + param->offset, param->value, final_len);

                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = s_char2_buff + param->offset;

                ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
            }
        }
        else if (param->handle == s_char2_desc_attr_handle)
        {
            if (param->need_rsp)
            {
                rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
                rsp.attr_value.handle = param->handle;
                rsp.attr_value.offset = param->offset;
                rsp.attr_value.value = (uint8_t *)&s_char2_desc_buff + param->offset;

                if ((param->value[0] % 2) == 0)
                {
                    final_len = MIN_VALUE(param->len, sizeof(s_char2_desc_buff) - param->offset);

                    memcpy((uint8_t *)&s_char2_desc_buff + param->offset, param->value, final_len);

                    rsp.attr_value.len = final_len;

                    ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
                }
                else
                {
                    gatt_loge("this attr %d must write with even num [0], not %d", param->handle, param->value[0]);
                    ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_PARAM_VAL_NOT_ALLOW, &rsp);
                }
            }
        }
        else if (param->handle == s_char_auto_rsp_attr_handle)
        {

        }
    }
    break;

    case BK_GATTS_EXEC_WRITE_EVT:
    {
        struct gatts_exec_write_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_EXEC_WRITE_EVT %d %d %d %d", param->conn_id, param->trans_id, param->exec_write_flag, param->need_rsp);

        if (param->need_rsp)
        {
            bk_gatt_rsp_t rsp;

            memset(&rsp, 0, sizeof(rsp));

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
        }
    }
    break;

    case BK_GATTS_CONF_EVT:
    {
        struct gatts_conf_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_CONF_EVT %d %d %d", param->status, param->conn_id, param->handle);
    }
    break;

    case BK_GATTS_RESPONSE_EVT: //todo done: internal resp, don't report to app.
    {
        struct gatts_rsp_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_RESPONSE_EVT %d %d", param->status, param->handle);
    }
    break;

    case BK_GATTS_SEND_SERVICE_CHANGE_EVT:
    {
        struct gatts_send_service_change_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_SEND_SERVICE_CHANGE_EVT %02x:%02x:%02x:%02x:%02x:%02x %d %d",
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0],
                  param->status, param->conn_id);
    }
    break;

    case BK_GATTS_CONNECT_EVT:
    {
        struct gatts_connect_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_CONNECT_EVT %d role %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id, param->link_role,
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0]);

        common_env_tmp = dm_ble_alloc_app_env_by_addr(param->remote_bda, sizeof(dm_gatts_app_env_t));

        if (!common_env_tmp)
        {
            gatt_loge("alloc app env err !!!!");
            break;
        }

        common_env_tmp->status = GAP_CONNECT_STATUS_CONNECTED;
        common_env_tmp->conn_id = param->conn_id;
        common_env_tmp->local_is_master = (param->link_role == 0 ? 1 : 0);

        s_conn_id = param->conn_id;
    }
    break;

    case BK_GATTS_DISCONNECT_EVT:
    {
        struct gatts_disconnect_evt_param *param = (typeof(param))comm_param;
        uint8_t timer_ref_count = 0;

        gatt_logi("BK_GATTS_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0]);

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            gatt_loge("not found addr !!!!");
            //break;
        }
        else
        {
            dm_ble_del_app_env_by_addr(param->remote_bda);
        }

        s_conn_id = 0xff;

        dm_ble_app_env_foreach(nest_func_check_timer_ref_count, &timer_ref_count);

        if (!timer_ref_count && rtos_is_timer_init(&s_char_notify_timer))
        {
            if (rtos_is_timer_running(&s_char_notify_timer))
            {
                rtos_stop_timer(&s_char_notify_timer);
            }

            rtos_deinit_timer(&s_char_notify_timer);
        }

#if GAP_IS_OLD_API
        ret = bk_ble_set_advertising_enable(1, dm_ble_cmd_cb);

        if (ret != BK_ERR_BLE_SUCCESS)
        {
            gatt_loge("bk_ble_set_advertising_enable err %d", ret);
        }

        if (s_ble_sema != NULL)
        {
            ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

            if (ret != kNoErr)
            {
                gatt_loge("rtos_get_semaphore 4 err %d", ret);
            }
        }

#else
        bk_bd_addr_t nominal_addr = {0};
        uint8_t nominal_addr_type = 0;
        bk_bd_addr_t identity_addr = {0};
        uint8_t identity_addr_type = 0;

        const bk_ble_gap_ext_adv_t ext_adv =
        {
            .instance = 0,
            .duration = 0,
            .max_events = 0,
        };

        if (0 && 0 == dm_gatt_get_authen_status(nominal_addr, &nominal_addr_type, identity_addr, &identity_addr_type))
        {
            // for bonded adv
            bk_ble_gap_ext_adv_params_t adv_param =
            {
                .type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
                .interval_min = 120,
                .interval_max = 160,
                .channel_map = BK_ADV_CHNL_ALL,
                .own_addr_type = BLE_ADDR_TYPE_RPA_RANDOM,
                .peer_addr_type = identity_addr_type,
                .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                .primary_phy = BK_BLE_GAP_PRI_PHY_1M,
                .secondary_phy = BK_BLE_GAP_PHY_1M,
                .sid = 0,
                .scan_req_notif = 0,
            };

            adv_param.peer_addr_type = identity_addr_type;
            os_memcpy(adv_param.peer_addr, identity_addr, sizeof(identity_addr));

            ret = bk_ble_gap_set_adv_params(0, &adv_param);

            if (ret)
            {
                gatt_loge("bk_ble_gap_set_adv_params err %d", ret);
                break;
            }

            rtos_delay_milliseconds(100);
        }

        ret = bk_ble_gap_adv_start(1, &ext_adv);

        if (ret)
        {
            gatt_loge("bk_ble_gap_adv_start err %d", ret);
        }

        rtos_delay_milliseconds(100);
#endif
    }
    break;

    case BK_GATTS_MTU_EVT:
    {
        struct gatts_mtu_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTS_MTU_EVT %d %d", param->conn_id, param->mtu);
    }
    break;

    default:
        break;
    }

    return ret;
}

int32_t dm_gatts_disconnect(uint8_t *addr)
{
    dm_gatt_app_env_t *common_env_tmp = NULL;

    gatt_logi("0x%02x:%02x:%02x:%02x:%02x:%02x",
              addr[5],
              addr[4],
              addr[3],
              addr[2],
              addr[1],
              addr[0]);

    common_env_tmp = dm_ble_find_app_env_by_addr(addr);

    if (!common_env_tmp || !common_env_tmp->data)
    {
        gatt_loge("conn max %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp->data : NULL);
        return -1;
    }

    if (common_env_tmp->status != GAP_CONNECT_STATUS_CONNECTED)
    {
        gatt_loge("connect status is not connected %d", common_env_tmp->status);
        return -1;
    }

    bd_addr_t connect_addr;
    int32_t err = 0;

    os_memcpy(connect_addr.addr, addr, sizeof(connect_addr.addr));
    //todo: use new api
    err = bk_ble_disconnect_connection(&connect_addr, NULL);

    if (err)
    {
        gatt_loge("disconnect fail %d", err);
    }
    else
    {
        common_env_tmp->status = GAP_CONNECT_STATUS_DISCONNECTING;
    }

    return err;
}

int32_t dm_gatts_start_adv(void)
{
    int32_t ret = 0;

    const bk_ble_gap_ext_adv_t ext_adv =
    {
        .instance = 0,
        .duration = 0,
        .max_events = 0,
    };

    ret = bk_ble_gap_adv_start(1, &ext_adv);

    if (ret)
    {
        gatt_loge("bk_ble_gap_adv_start err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set adv enable err %d", ret);
        goto error;
    }

    return 0;
error:;
    gatt_loge("err");
    return -1;
}

int32_t dm_gatts_enable_service(uint32_t index, uint8_t enable)
{
    int32_t ret = 0;
    uint16_t handle = (index == 0 ? s_service_attr_handle : s_attr_handle_list2[0]);

    if (enable)
    {
        bk_ble_gatts_start_service(handle);
    }
    else
    {
        bk_ble_gatts_stop_service(handle);
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait start/stop service err %d", ret);
        return -1;
    }

    bk_ble_gatts_send_service_change_indicate(s_gatts_if, 0, 1);

    return 0;
}

int dm_gatts_main(void)
{
    ble_err_t ret = 0;

    ret = rtos_init_semaphore(&s_ble_sema, 1);

    if (ret != 0)
    {
        gatt_loge("rtos_init_semaphore err %d", ret);
        return -1;
    }

    bk_ble_gatts_register_callback(bk_gatts_cb);

    ret = bk_ble_gatts_app_register(0);

    if (ret)
    {
        gatt_loge("reg err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore reg err %d", ret);
        return -1;
    }

#if DYNAMIC_ADD_ATTR

    bk_bt_uuid_t uuid;
    const bk_gatt_perm_t perm = BK_GATT_PERM_READ | BK_GATT_PERM_WRITE;
    bk_attr_value_t value;
    bk_attr_control_t ctrl;
    const bk_gatt_char_prop_t prop = BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE
                                     | BK_GATT_CHAR_PROP_BIT_NOTIFY;

    bk_gatt_srvc_id_t srvc_id;

    memset(&srvc_id, 0, sizeof(srvc_id));
    srvc_id.is_primary = 1;
    srvc_id.id.uuid.len = BK_UUID_LEN_16;
    srvc_id.id.uuid.uuid.uuid16 = 0x1234;

    ret = bk_ble_gatts_create_service(0, &srvc_id, 30);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_create_service err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }


    memset(&uuid, 0, sizeof(uuid));
    memset(&value, 0, sizeof(value));
    memset(&ctrl, 0, sizeof(ctrl));

    uuid.len = BK_UUID_LEN_16;
    uuid.uuid.uuid16 = 0x5678;
    value.attr_value = s_char_buff;
    value.attr_len = value.attr_max_len = sizeof(s_char_buff);
    ctrl.auto_rsp = 0;

    ret = bk_ble_gatts_add_char(s_service_attr_handle, &uuid, perm, prop, &value, &ctrl);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_add_char err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    {
        uint16_t tmp_len = 0;
        uint8_t *tmp_buff = NULL;
        bk_gatt_status_t g_status = 0;

        g_status = bk_ble_gatts_get_attr_value(s_char_attr_handle, &tmp_len, &tmp_buff);

        if (g_status)
        {
            gatt_loge("get attr value err %d", g_status);
        }

        if (tmp_len != value.attr_len || tmp_buff != value.attr_value)
        {
            gatt_loge("%d %d get attr value not match create %d %d %p %p!!!!", __LINE__, g_status,
                      tmp_len, value.attr_len, tmp_buff, value.attr_value);
        }
    }

    memset(&uuid, 0, sizeof(uuid));
    memset(&value, 0, sizeof(value));
    memset(&ctrl, 0, sizeof(ctrl));

    uuid.len = BK_UUID_LEN_16;
    uuid.uuid.uuid16 = BK_GATT_UUID_CHAR_CLIENT_CONFIG;
    value.attr_value = (uint8_t *)&s_char_desc_buff;
    value.attr_len = value.attr_max_len = sizeof(s_char_desc_buff);
    ctrl.auto_rsp = 0;

    ret = bk_ble_gatts_add_char_descr(s_service_attr_handle, s_char_attr_handle, &uuid, perm, &value, &ctrl);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_add_char_descr err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    {
        uint16_t tmp_len = 0;
        uint8_t *tmp_buff = NULL;
        bk_gatt_status_t g_status = 0;

        g_status = bk_ble_gatts_get_attr_value(s_char_desc_attr_handle, &tmp_len, &tmp_buff);

        if (g_status)
        {
            gatt_loge("get attr value err %d", g_status);
        }

        if (tmp_len != value.attr_len || tmp_buff != value.attr_value)
        {
            gatt_loge("%d %d get attr value not match create %d %d %p %p!!!!", __LINE__, g_status,
                      tmp_len, value.attr_len, tmp_buff, value.attr_value);
        }
    }

    memset(&uuid, 0, sizeof(uuid));
    memset(&value, 0, sizeof(value));
    memset(&ctrl, 0, sizeof(ctrl));

    uuid.len = BK_UUID_LEN_16;
    uuid.uuid.uuid16 = 0x9abc;
    value.attr_value = s_char2_buff;
    value.attr_len = value.attr_max_len = sizeof(s_char2_buff);
    ctrl.auto_rsp = 0;

    ret = bk_ble_gatts_add_char(s_service_attr_handle, &uuid, perm, prop, &value, &ctrl);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_add_char 2 err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    {
        uint16_t tmp_len = 0;
        uint8_t *tmp_buff = NULL;
        bk_gatt_status_t g_status = 0;

        g_status = bk_ble_gatts_get_attr_value(s_char2_attr_handle, &tmp_len, &tmp_buff);

        if (g_status)
        {
            gatt_loge("get attr value err %d", g_status);
        }

        if (tmp_len != value.attr_len || tmp_buff != value.attr_value)
        {
            gatt_loge("%d %d get attr value not match create %d %d %p %p!!!!", __LINE__, g_status,
                      tmp_len, value.attr_len, tmp_buff, value.attr_value);
        }
    }

    memset(&uuid, 0, sizeof(uuid));
    memset(&value, 0, sizeof(value));
    memset(&ctrl, 0, sizeof(ctrl));

    uuid.len = BK_UUID_LEN_16;
    uuid.uuid.uuid16 = BK_GATT_UUID_CHAR_SRVR_CONFIG;
    value.attr_value = (uint8_t *)&s_char2_desc_buff;
    value.attr_len = value.attr_max_len = sizeof(s_char2_desc_buff);
    ctrl.auto_rsp = 0;

    ret = bk_ble_gatts_add_char_descr(s_service_attr_handle, s_char2_attr_handle, &uuid, perm, &value, &ctrl);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_add_char_descr 2 err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }


    {
        uint16_t tmp_len = 0;
        uint8_t *tmp_buff = NULL;
        bk_gatt_status_t g_status = 0;

        g_status = bk_ble_gatts_get_attr_value(s_char2_desc_attr_handle, &tmp_len, &tmp_buff);

        if (g_status)
        {
            gatt_loge("get attr value err %d", g_status);
        }

        if (tmp_len != value.attr_len || tmp_buff != value.attr_value)
        {
            gatt_loge("%d %d get attr value not match create %d %d %p %p!!!!", __LINE__, g_status,
                      tmp_len, value.attr_len, tmp_buff, value.attr_value);
        }
    }


    memset(&uuid, 0, sizeof(uuid));
    memset(&value, 0, sizeof(value));
    memset(&ctrl, 0, sizeof(ctrl));

    uuid.len = BK_UUID_LEN_16;
    uuid.uuid.uuid16 = 0x15ab;
    value.attr_value = s_char_auto_rsp_buff;
    value.attr_len = value.attr_max_len = sizeof(s_char_auto_rsp_buff);
    ctrl.auto_rsp = 1;

    ret = bk_ble_gatts_add_char(s_service_attr_handle, &uuid, perm, prop, &value, &ctrl);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_add_char auto err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    {
        uint16_t tmp_len = 0;
        uint8_t *tmp_buff = NULL;
        bk_gatt_status_t g_status = 0;

        g_status = bk_ble_gatts_get_attr_value(s_char_auto_rsp_attr_handle, &tmp_len, &tmp_buff);

        if (g_status)
        {
            gatt_loge("get attr value err %d", g_status);
        }

        if (tmp_len != value.attr_len || tmp_buff != value.attr_value)
        {
            gatt_loge("%d %d get attr value not match create %d %d %p %p!!!!", __LINE__, g_status,
                      tmp_len, value.attr_len, tmp_buff, value.attr_value);
        }
    }


#else

    ret = bk_ble_gatts_create_attr_tab(s_gatts_attr_db_service_1, s_gatts_if, sizeof(s_gatts_attr_db_service_1) / sizeof(s_gatts_attr_db_service_1[0]), 30);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_create_attr_tab err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    ret = bk_ble_gatts_create_attr_tab(s_gatts_attr_db_service_2, s_gatts_if, sizeof(s_gatts_attr_db_service_2) / sizeof(s_gatts_attr_db_service_2[0]), 30);

    if (ret != 0)
    {
        gatt_loge("bk_ble_gatts_create_attr_tab 2 err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

#endif

    bk_ble_gatts_start_service(s_service_attr_handle);

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    bk_ble_gatts_start_service(s_attr_handle_list2[0]);

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

#if GAP_IS_OLD_API

    bd_addr_t random_addr;
    bk_get_mac((uint8_t *)random_addr.addr, MAC_TYPE_BLUETOOTH);

    for (int i = 0; i < sizeof(random_addr.addr) / 2; i++)
    {
        uint8_t tmp_addr = random_addr.addr[i];
        random_addr.addr[i] = random_addr.addr[sizeof(random_addr.addr) - 1 - i];
        random_addr.addr[sizeof(random_addr.addr) - 1 - i] = tmp_addr;
    }

    random_addr.addr[0]++;

    ble_adv_parameter_t tmp_param;

    uint8_t adv_data[31] =
    {
        //adv format <len> <type> <payload>, len = type + payload, type pls see <<Generic Access Profile>>'s Assigned number

        //len = 0x2, type = 1 means Flags
        0x02, 0x01, 0x06,

        //len = strlen(name) + 1, type = 0x8 means Shortened Local Name, "SMART-SOUNDBAR"
        1 + 0, 0x08, //name suchas 'S', 'M', 'A', 'R', 'T', '-', 'S', 'O', 'U', 'N', 'D', 'B', 'A', 'R',
    };

    uint8_t adv_name_len = snprintf((char *)(adv_data + 5), sizeof(adv_data) - 5, "CENTRAL-%02X%02X%02X", random_addr.addr[2], random_addr.addr[1], random_addr.addr[0]);

    adv_data[3] = adv_name_len + 1;

    uint8_t adv_len = 5 + adv_name_len;

    extern uint8_t test_adv_data_len[31 - sizeof(adv_data)];  //attention: sizeof(adv_data) must <= 31 !!!!!!!!!!!!!
    (void)test_adv_data_len;

    memset(&tmp_param, 0, sizeof(tmp_param));

    tmp_param.adv_intv_max = 160;
    tmp_param.adv_intv_min = 120;
    tmp_param.adv_type = ADV_LEGACY_TYPE_ADV_IND;
    tmp_param.chnl_map = ADV_ALL_CHNLS;
    tmp_param.filter_policy = ADV_FILTER_POLICY_ALLOW_SCAN_ANY_CONNECT_ANY;
    tmp_param.own_addr_type = 1;//0;
    tmp_param.peer_addr_type = 0;
    //tmp_param.peer_addr;

    //    ret = bk_ble_set_advertising_params(adv_param.adv_intv_min, adv_param.adv_intv_max, adv_param.chnl_map,
    //            adv_param.own_addr_type,adv_param.prim_phy, adv_param.second_phy, ble_at_cmd_cb);

    gatt_logi("bk_ble_set_advertising_params start", ret);
    ret = bk_ble_set_advertising_params(&tmp_param, dm_ble_cmd_cb);


    if (ret != BK_ERR_BLE_SUCCESS)
    {
        gatt_loge("bk_ble_set_advertising_params err %d", ret);
        goto error;
    }

    gatt_logi("bk_ble_set_advertising_params wait", ret);

    if (s_ble_sema != NULL)
    {
        ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

        if (ret != kNoErr)
        {
            gatt_loge("rtos_get_semaphore 1 err %d", ret);
            goto error;
        }
    }

    gatt_logi("bk_ble_set_random_addr start", ret);
    ret = bk_ble_set_random_addr((bd_addr_t *)&random_addr, dm_ble_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        gatt_loge("bk_ble_set_random_addr err %d", ret);
        goto error;
    }

    gatt_logi("bk_ble_set_random_addr wait", ret);

    if (s_ble_sema != NULL)
    {
        ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

        if (ret != kNoErr)
        {
            gatt_loge("rtos_get_semaphore 2 err %d", ret);
            goto error;
        }
    }

    ret = bk_ble_set_advertising_data(adv_len, (uint8_t *)adv_data, dm_ble_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        gatt_loge("bk_ble_set_advertising_data err %d", ret);
        goto error;
    }

    if (s_ble_sema != NULL)
    {
        ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

        if (ret != kNoErr)
        {
            gatt_loge("rtos_get_semaphore 3 err %d", ret);
            goto error;
        }
    }

    ret = bk_ble_set_advertising_enable(1, dm_ble_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        gatt_loge("bk_ble_set_advertising_enable err %d", ret);
        goto error;
    }

    if (s_ble_sema != NULL)
    {
        ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

        if (ret != kNoErr)
        {
            gatt_loge("rtos_get_semaphore 4 err %d", ret);
            goto error;
        }
    }

#else
    bk_bd_addr_t random_addr;
    char adv_name[64] = {0};

    bk_get_mac((uint8_t *)random_addr, MAC_TYPE_BLUETOOTH);

    for (int i = 0; i < sizeof(random_addr) / 2; i++)
    {
        uint8_t tmp_addr = random_addr[i];
        random_addr[i] = random_addr[sizeof(random_addr) - 1 - i];
        random_addr[sizeof(random_addr) - 1 - i] = tmp_addr;
    }

    random_addr[0]++;
    random_addr[5] |= 0xc0; // static random addr[47:46] must be 0b11 in msb !!!

    dm_gatt_add_gap_callback(dm_ble_gap_cb);

    snprintf((char *)(adv_name), sizeof(adv_name) - 1, "CENTRAL-%02X%02X%02X", random_addr[2], random_addr[1], random_addr[0]);

    ret = bk_ble_gap_set_device_name(adv_name);

    if (ret)
    {
        gatt_loge("bk_ble_gap_set_device_name err %d", ret);
        goto error;
    }

    bk_ble_gap_ext_adv_params_t adv_param =
    {
        .type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
        .interval_min = 120,
        .interval_max = 160,
        .channel_map = BK_ADV_CHNL_ALL,
        .own_addr_type = BLE_ADDR_TYPE_RANDOM,//BLE_ADDR_TYPE_RANDOM,
        .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        .primary_phy = BK_BLE_GAP_PRI_PHY_1M,
        .secondary_phy = BK_BLE_GAP_PHY_1M,
        .sid = 0,
        .scan_req_notif = 0,
    };

    ret = bk_ble_gap_set_adv_params(0, &adv_param);

    if (ret)
    {
        gatt_loge("bk_ble_gap_set_adv_params err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set adv param err %d", ret);
        goto error;
    }

    ret = bk_ble_gap_set_rand_addr(random_addr);

    if (ret)
    {
        gatt_loge("bk_ble_gap_set_rand_addr err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set rand addr err %d", ret);
        goto error;
    }

    if (adv_param.own_addr_type == BLE_ADDR_TYPE_RANDOM)
    {
        ret = bk_ble_gap_set_adv_rand_addr(0, random_addr);

        if (ret)
        {
            gatt_loge("bk_ble_gap_set_adv_rand_addr err %d", ret);
            goto error;
        }

        ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

        if (ret != kNoErr)
        {
            gatt_loge("wait set adv rand addr err %d", ret);
            goto error;
        }
    }

    bk_ble_adv_data_t adv_data =
    {
        .set_scan_rsp = 0,
        .include_name = 1,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = 0x06,
    };

    ret = bk_ble_gap_set_adv_data((bk_ble_adv_data_t *)&adv_data);

    if (ret)
    {
        gatt_loge("bk_ble_gap_set_adv_data err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set adv data err %d", ret);
        goto error;
    }

    adv_data.set_scan_rsp = 1;

    ret = bk_ble_gap_set_adv_data((bk_ble_adv_data_t *)&adv_data);

    if (ret)
    {
        gatt_loge("bk_ble_gap_set_adv_data scan rsp err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set adv data scan rsp err %d", ret);
        goto error;
    }

    const bk_ble_gap_ext_adv_t ext_adv =
    {
        .instance = 0,
        .duration = 0,
        .max_events = 0,
    };

    ret = bk_ble_gap_adv_start(1, &ext_adv);

    if (ret)
    {
        gatt_loge("bk_ble_gap_adv_start err %d", ret);
        goto error;
    }

    ret = rtos_get_semaphore(&s_ble_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        gatt_loge("wait set adv enable err %d", ret);
        goto error;
    }

#endif

error:
    return 0;
}
