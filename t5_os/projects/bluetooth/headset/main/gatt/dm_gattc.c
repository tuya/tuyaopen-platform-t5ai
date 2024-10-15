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

#include "components/bluetooth/bk_dm_ble.h"
#include "dm_gattc.h"
#include "dm_gatt_connection.h"

#define DYNAMIC_ADD_ATTR 0
//#define SPECIAL_HANDLE 4 //xunfei

#define SPECIAL_HANDLE 15 //7256 server


#define INVALID_ATTR_HANDLE 0
#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))

enum
{
    GATTC_STATUS_IDLE,
    GATTC_STATUS_DISCOVERY_SELF,
    GATTC_STATUS_READ_BY_TYPE,
    GATTC_STATUS_READ_CHAR,
    GATTC_STATUS_READ_CHAR_DESC,
    GATTC_STATUS_READ_MULTI,
    GATTC_STATUS_WRITE_DESC_NEED_RSP,
    GATTC_STATUS_WRITE_DESC_NO_RSP,
    GATTC_STATUS_PREP_WRITE_STEP_1,
    GATTC_STATUS_PREP_WRITE_STEP_2,
    GATTC_STATUS_WRITE_EXEC,
    GATTC_STATUS_WRITE_READ_SAMETIME,
};


//typedef struct
//{
//    uint8_t job_status; //see GATTC_STATUS_IDLE
//    uint8_t noti_indica_switch;
//    uint8_t noti_indicate_recv_count;
//} dm_gattc_app_env_t;

#define dm_gattc_app_env_t dm_gatt_addition_app_env_t

//static dm_gattc_app_env_t s_dm_gattc_app_env_array[GATT_MAX_CONNECTION_COUNT];

static bk_gatt_if_t s_gattc_if;
static beken_semaphore_t s_ble_sema = NULL;

static uint16_t s_peer_service_start_handle = 0;
static uint16_t s_peer_service_end_handle = 0;
static uint16_t s_peer_service_char_handle = 0;
static uint16_t s_peer_service_char_desc_handle = 0;

static int32_t bk_gattc_cb (bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    bk_gatt_auth_req_t auth_req = BK_GATT_AUTH_REQ_NONE;
    dm_gattc_app_env_t *app_env_tmp = NULL;
    dm_gatt_app_env_t *common_env_tmp = NULL;

    const uint16_t client_config_noti_enable = 1, client_config_indic_enable = 2, client_config_all_disable = 0;

    switch (event)
    {
    case BK_GATTC_REG_EVT:
    {
        struct gattc_reg_evt_param *param = (typeof(param))comm_param;

        s_gattc_if = param->gatt_if;
        gatt_logi("reg ret gatt_if %d", s_gattc_if);

        if (s_ble_sema != NULL)
        {
            rtos_set_semaphore( &s_ble_sema );
        }
    }

    break;

    case BK_GATTC_DIS_SRVC_CMPL_EVT:
    {
        struct gattc_dis_srvc_cmpl_evt_param *param = (typeof(param))comm_param;

        bk_bt_uuid_t uuid = {BK_UUID_LEN_16, {BK_GATT_UUID_CHAR_DECLARE}};

        gatt_logi("BK_GATTC_DIS_SRVC_CMPL_EVT %d %d", param->status, param->conn_id);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        gatt_logi("job_status %d", app_env_tmp->job_status);

        if (app_env_tmp->job_status == GATTC_STATUS_IDLE)
        {
            if (0 != bk_ble_gattc_discover(s_gattc_if, param->conn_id, auth_req))
            {
                gatt_loge("bk_ble_gattc_discover err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_DISCOVERY_SELF;
            }
        }
        else if (app_env_tmp->job_status == GATTC_STATUS_DISCOVERY_SELF)
        {
            if (0 != bk_ble_gattc_read_by_type(s_gattc_if, param->conn_id, 1, 98, &uuid, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_by_type err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_READ_BY_TYPE;
            }
        }
    }
    break;

    case BK_GATTC_DIS_RES_SERVICE_EVT:
    {
        struct gattc_dis_res_service_evt_param *param = (typeof(param))comm_param;
        uint16_t short_uuid = 0;

        gatt_logi("BK_GATTC_DIS_RES_SERVICE_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].srvc_id.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                short_uuid = param->array[i].srvc_id.uuid.uuid.uuid16;
            }
            break;

            case BK_UUID_LEN_128:
            {
                memcpy(&short_uuid, &param->array[i].srvc_id.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
            }
            break;
            }

            gatt_logi("0x%04x %d~%d", short_uuid, param->array[i].start_handle, param->array[i].end_handle);

            if (0x1234 == short_uuid)
            {
                s_peer_service_start_handle = param->array[i].start_handle;
                s_peer_service_end_handle = param->array[i].end_handle;

                gatt_logi("interesting service %d~%d", s_peer_service_start_handle, s_peer_service_end_handle);
            }
        }
    }
    break;

    case BK_GATTC_DIS_RES_CHAR_EVT:
    {
        struct gattc_dis_res_char_evt_param *param = (typeof(param))comm_param;
        uint16_t short_uuid = 0;

        gatt_logi("BK_GATTC_DIS_RES_CHAR_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].uuid.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                short_uuid = param->array[i].uuid.uuid.uuid.uuid16;
            }
            break;

            case BK_UUID_LEN_128:
            {
                memcpy(&short_uuid, &param->array[i].uuid.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
            }
            break;
            }

            gatt_logi("0x%04x %d~%d char_value_handle %d", short_uuid, param->array[i].start_handle, param->array[i].end_handle, param->array[i].char_value_handle);

            if ( s_peer_service_start_handle <= param->array[i].char_value_handle &&
                    s_peer_service_end_handle >= param->array[i].char_value_handle &&
                    0x5678 == short_uuid)
            {
                s_peer_service_char_handle = param->array[i].char_value_handle;

                gatt_logi("interesting char %d", s_peer_service_char_handle);
            }
        }
    }
    break;

    case BK_GATTC_DIS_RES_CHAR_DESC_EVT:
    {
        struct gattc_dis_res_char_desc_evt_param *param = (typeof(param))comm_param;
        uint16_t short_uuid = 0;

        gatt_logi("BK_GATTC_DIS_RES_CHAR_DESC_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].uuid.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                short_uuid = param->array[i].uuid.uuid.uuid.uuid16;
            }
            break;

            case BK_UUID_LEN_128:
            {
                memcpy(&short_uuid, &param->array[i].uuid.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
            }
            break;

            default:
                gatt_loge("unknow uuid len %d", param->array[i].uuid.uuid.len);
                break;
            }

            gatt_logi("0x%04x char_handle %d desc_handle %d", short_uuid, param->array[i].char_handle, param->array[i].desc_handle);

            if (BK_GATT_UUID_CHAR_CLIENT_CONFIG == short_uuid &&
                    s_peer_service_char_handle == param->array[i].char_handle)
            {
                gatt_logi("interesting char desc %d", param->array[i].desc_handle);

                //if don't want enable notify, remove this.
                s_peer_service_char_desc_handle = param->array[i].desc_handle;
            }
        }
    }
    break;

    case BK_GATTC_READ_CHAR_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_READ_CHAR_EVT 0x%x %d %d", param->status, param->handle, param->value_len);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (!param->status && app_env_tmp->job_status == GATTC_STATUS_READ_CHAR)
        {
            if (0 != bk_ble_gattc_read_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_char_descr err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_READ_CHAR_DESC;
            }
        }
    }
    break;

    case BK_GATTC_READ_DESCR_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;
        bk_gattc_multi_t multi;

        gatt_logi("BK_GATTC_READ_DESCR_EVT %x %d %d", param->status, param->handle, param->value_len);

        memset(&multi, 0, sizeof(multi));

        multi.num_attr = 5;
        multi.handles[0] = 3;
        multi.handles[1] = 5;
        multi.handles[2] = 9;
        multi.handles[3] = 13;
        multi.handles[4] = 16;

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_READ_CHAR_DESC)
        {
            if (0 != bk_ble_gattc_read_multiple(s_gattc_if, param->conn_id, &multi, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_multiple err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_READ_MULTI;
            }
        }
    }
    break;

    case BK_GATTC_READ_BY_TYPE_EVT:
    {
        struct gattc_read_by_type_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_READ_BY_TYPE_EVT %d %d %d", param->status, param->conn_id, param->elem_count);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_READ_BY_TYPE)
        {
            if (0 != bk_ble_gattc_read_char(s_gattc_if, param->conn_id, 3, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_char err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_READ_CHAR;
            }
        }
    }
    break;

    case BK_GATTC_READ_MULTIPLE_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_READ_MULTIPLE_EVT %x %d %d", param->status, param->handle, param->value_len);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_READ_MULTI)
        {
            if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, s_peer_service_char_desc_handle, sizeof(client_config_noti_enable), (uint8_t *)&client_config_noti_enable, BK_GATT_WRITE_TYPE_RSP, auth_req))
            {
                gatt_loge("bk_ble_gattc_write_char_descr err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_WRITE_DESC_NEED_RSP;
            }
        }
    }
    break;

    case BK_GATTC_WRITE_CHAR_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_WRITE_CHAR_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);

        if (param->status == BK_GATT_INSUF_AUTHENTICATION)
        {
            //we need create bond
            gatt_logw("status insufficient authentication, need bond !!!");
        }
    }
    break;

    case BK_GATTC_WRITE_DESCR_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_WRITE_DESCR_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);

        if (param->status == BK_GATT_INSUF_AUTHENTICATION)
        {
            //we need create bond
            gatt_logw("status insufficient authentication, need bond !!!");
        }

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_WRITE_DESC_NEED_RSP)
        {
            if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, s_peer_service_char_desc_handle, sizeof(client_config_all_disable), (uint8_t *)&client_config_all_disable, BK_GATT_WRITE_TYPE_NO_RSP, auth_req))
            {
                gatt_loge("bk_ble_gattc_write_char_descr err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_WRITE_DESC_NO_RSP;
            }
        }
        else if (app_env_tmp->job_status == GATTC_STATUS_WRITE_DESC_NO_RSP)
        {
            uint8_t buff = 1;

            if (0 != bk_ble_gattc_prepare_write(s_gattc_if, param->conn_id, SPECIAL_HANDLE, 0, sizeof(buff), (uint8_t *)&buff, auth_req))
            {
                gatt_loge("bk_ble_gattc_prepare_write err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_PREP_WRITE_STEP_1;
            }
        }
    }
    break;

    case BK_GATTC_PREP_WRITE_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_PREP_WRITE_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_PREP_WRITE_STEP_1)
        {
            uint8_t buff = 0;

            if (0 != bk_ble_gattc_prepare_write(s_gattc_if, param->conn_id, SPECIAL_HANDLE, 1, /*400,*/sizeof(buff), (uint8_t *)&buff, auth_req))
            {
                gatt_loge("bk_ble_gattc_prepare_write err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_PREP_WRITE_STEP_2;
            }
        }
        else if (app_env_tmp->job_status == GATTC_STATUS_PREP_WRITE_STEP_2)
        {
            if (0 != bk_ble_gattc_execute_write(s_gattc_if, param->conn_id, 1))
            {
                gatt_loge("bk_ble_gattc_execute_write err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_WRITE_EXEC;
            }
        }
    }
    break;

    case BK_GATTC_EXEC_EVT:
    {
        struct gattc_exec_cmpl_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_EXEC_EVT %d %d", param->status, param->conn_id);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_WRITE_EXEC)
        {
            //consecutive write read test
            if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, sizeof(client_config_noti_enable), (uint8_t *)&client_config_noti_enable, BK_GATT_WRITE_TYPE_RSP, auth_req))
            {
                gatt_loge("bk_ble_gattc_write_char_descr 1 err");
            }
            else if (0 != bk_ble_gattc_read_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_char_descr 2 err");
            }
            else if (0 != bk_ble_gattc_read_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, auth_req))
            {
                gatt_loge("bk_ble_gattc_read_char_descr 3 err");
            }
            else if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, sizeof(client_config_all_disable), (uint8_t *)&client_config_all_disable, BK_GATT_WRITE_TYPE_RSP, auth_req))
            {
                gatt_loge("bk_ble_gattc_write_char_descr 4 err");
            }
            else if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, SPECIAL_HANDLE, sizeof(client_config_noti_enable), (uint8_t *)&client_config_noti_enable, BK_GATT_WRITE_TYPE_NO_RSP, auth_req))
            {
                gatt_loge("bk_ble_gattc_write_char_descr 5 err");
            }
            else
            {
                app_env_tmp->job_status = GATTC_STATUS_WRITE_READ_SAMETIME;
            }
        }
    }
    break;

    case BK_GATTC_NOTIFY_EVT:
    {
        struct gattc_notify_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_NOTIFY_EVT %d %d handle %d vallen %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id,
                  param->is_notify,
                  param->handle,
                  param->value_len,
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0]);

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("conn_id %d not found %d %p", param->conn_id, common_env_tmp->data);
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->data;

        if (app_env_tmp->job_status == GATTC_STATUS_WRITE_READ_SAMETIME && app_env_tmp->noti_indicate_recv_count++ >= 3)
        {
            app_env_tmp->noti_indicate_recv_count = 0;

            if (app_env_tmp->noti_indica_switch)
            {
                ret = bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, s_peer_service_char_desc_handle, sizeof(client_config_noti_enable), (uint8_t *)&client_config_noti_enable, BK_GATT_WRITE_TYPE_RSP, auth_req);
            }
            else
            {
                ret = bk_ble_gattc_write_char_descr(s_gattc_if, param->conn_id, s_peer_service_char_desc_handle, sizeof(client_config_indic_enable), (uint8_t *)&client_config_indic_enable, BK_GATT_WRITE_TYPE_RSP, auth_req);
            }

            app_env_tmp->noti_indica_switch = (app_env_tmp->noti_indica_switch + 1) % 2;

            if (ret)
            {
                gatt_loge("bk_ble_gattc_write_char_descr switch err");
            }
        }
    }
    break;

    case BK_GATTC_CFG_MTU_EVT:
    {
        struct gattc_cfg_mtu_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_CFG_MTU_EVT status %d %d %d", param->status, param->conn_id, param->mtu);

        if (0 != bk_ble_gattc_discover(s_gattc_if, param->conn_id, auth_req))
        {
            gatt_loge("bk_ble_gattc_discover err");
        }
    }
    break;

    case BK_GATTC_CONNECT_EVT:
    {
        struct gattc_connect_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_CONNECT_EVT %d %02X:%02X:%02X:%02X:%02X:%02X %d", param->link_role,
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0],
                  param->conn_id);

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            gatt_logw("not found addr, alloc it !!!!");
            common_env_tmp = dm_ble_alloc_app_env_by_addr(param->remote_bda, sizeof(dm_gattc_app_env_t));

            if (!common_env_tmp || !common_env_tmp->data)
            {
                gatt_loge("conn max %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp->data : NULL);
                break;
            }

            if (common_env_tmp->status != GAP_CONNECT_STATUS_IDLE)
            {
                gatt_loge("connect status is not idle %d", common_env_tmp->status);
                break;
            }
        }

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("not found addr or data %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp->data : NULL);
            break;
        }

        common_env_tmp->status = GAP_CONNECT_STATUS_CONNECTED;
        common_env_tmp->conn_id = param->conn_id;
        common_env_tmp->local_is_master = (param->link_role == 0 ? 1 : 0);

        gatt_logi("local role %d", common_env_tmp->local_is_master);

        if (common_env_tmp->local_is_master)
        {
            // only do mtu req when local is master
            if (0 != bk_ble_gattc_send_mtu_req(s_gattc_if, common_env_tmp->conn_id))
            {
                gatt_loge("bk_ble_gattc_send_mtu_req err");
            }
        }
        else
        {
            if (0 != bk_ble_gattc_discover(s_gattc_if, param->conn_id, auth_req))
            {
                gatt_loge("bk_ble_gattc_discover err");
            }
        }
    }
    break;

    case BK_GATTC_DISCONNECT_EVT:
    {
        struct gattc_disconnect_evt_param *param = (typeof(param))comm_param;

        gatt_logi("BK_GATTC_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                  param->remote_bda[5],
                  param->remote_bda[4],
                  param->remote_bda[3],
                  param->remote_bda[2],
                  param->remote_bda[1],
                  param->remote_bda[0]);

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp || !common_env_tmp->data)
        {
            gatt_loge("not found addr or data %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp->data : NULL);
            break;
        }

        dm_ble_del_app_env_by_addr(param->remote_bda);
    }
    break;

    default:
        break;
    }

    return ret;
}

static uint32_t dm_ble_legacy_event_cb(ble_event_enum_t event, void *param)
{
    switch (event)
    {

    case BK_DM_BLE_EVENT_CONNECT:
    {
        ble_conn_att_t *conn_att = (typeof(conn_att)) param;
        gatt_logi("ethermind connected, ATT_CON_ID %d atype %d addr 0x%02X:%02X:%02X:%02X:%02X:%02X",
                  conn_att->conn_handle, conn_att->peer_addr_type,
                  conn_att->peer_addr[5], conn_att->peer_addr[4],
                  conn_att->peer_addr[3], conn_att->peer_addr[2],
                  conn_att->peer_addr[1], conn_att->peer_addr[0]);
    }
    break;

    case BK_DM_BLE_EVENT_DISCONNECT:
    {
        ble_conn_att_t *att_handle = (typeof(att_handle)) param;
        gatt_logi("ethermind disconnect reason %d attid %d atype %d addr 0x%02X:%02X:%02X:%02X:%02X:%02X",
                  att_handle->event_result, att_handle->conn_handle,
                  att_handle->peer_addr_type, att_handle->peer_addr[5],
                  att_handle->peer_addr[4], att_handle->peer_addr[3],
                  att_handle->peer_addr[2], att_handle->peer_addr[1],
                  att_handle->peer_addr[0]);
    }
    break;

    default:
        break;
    }

    return 0;
}

int32_t dm_gattc_connect(uint8_t *addr)
{
    dm_gatt_app_env_t *common_env_tmp = NULL;

    gatt_logi("0x%02x:%02x:%02x:%02x:%02x:%02x",
              addr[5],
              addr[4],
              addr[3],
              addr[2],
              addr[1],
              addr[0]);

    common_env_tmp = dm_ble_alloc_app_env_by_addr(addr, sizeof(dm_gattc_app_env_t));

    if (!common_env_tmp || !common_env_tmp->data)
    {
        gatt_loge("conn max %p %p !!!!", common_env_tmp, common_env_tmp ? common_env_tmp->data : NULL);
        return -1;
    }

    if (common_env_tmp->status != GAP_CONNECT_STATUS_IDLE)
    {
        gatt_loge("connect status is not idle %d", common_env_tmp->status);
        return -1;
    }

    //todo: use new connect api
    typedef struct
    {
        uint8_t    peer_address_type;
        bd_addr_t  peer_address;
        uint8_t    initiating_phys;

        uint16_t conn_interval_min;
        uint16_t conn_interval_max;
        uint16_t conn_latency;
        uint16_t supervision_timeout;
    } ble_conn_param_normal_t;


    ble_conn_param_normal_t tmp;
    int32_t err = 0;

    tmp.conn_interval_min = 6;
    tmp.conn_interval_max = 6;
    tmp.conn_latency = 0;
    tmp.supervision_timeout = 0x200;
    tmp.initiating_phys = 1;

    tmp.peer_address_type = 1;
    memcpy(tmp.peer_address.addr, addr, sizeof(tmp.peer_address.addr));

    //todo: not impl bk_ble_create_connection in ethermind
    bk_ble_set_event_callback(dm_ble_legacy_event_cb);
    extern ble_err_t bk_ble_create_connection(ble_conn_param_normal_t *conn_param, void *callback);
    err = bk_ble_create_connection(&tmp, NULL);

    if (err)
    {
        gatt_loge("connect fail %d", err);
    }
    else
    {
        os_memcpy(common_env_tmp->addr, addr, sizeof(common_env_tmp->addr));
        common_env_tmp->status = GAP_CONNECT_STATUS_CONNECTING;
    }

    return err;
}

int32_t dm_gattc_disconnect(uint8_t *addr)
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

int32_t dm_gattc_discover(uint16_t conn_id)
{
    if (bk_ble_gattc_discover(s_gattc_if, conn_id, BK_GATT_AUTH_REQ_NONE))
    {
        gatt_loge("err");
    }

    return 0;
}

//ble_gatt_demo gattc write 5 18 111111111111111111111
int32_t dm_gattc_write(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint32_t len)
{
    if (0 != bk_ble_gattc_write_char_descr(s_gattc_if, conn_id, attr_handle, len, data, BK_GATT_WRITE_TYPE_RSP, BK_GATT_AUTH_REQ_NONE))
    {
        gatt_loge("err");
    }

    return 0;
}

int dm_gattc_main(void)
{
    ble_err_t ret = 0;

    ret = rtos_init_semaphore(&s_ble_sema, 1);

    if (ret != 0)
    {
        gatt_loge("rtos_init_semaphore err %d", ret);
        return -1;
    }

    bk_ble_gattc_register_callback(bk_gattc_cb);

    ret = bk_ble_gattc_app_register(0);

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

    return 0;
}
