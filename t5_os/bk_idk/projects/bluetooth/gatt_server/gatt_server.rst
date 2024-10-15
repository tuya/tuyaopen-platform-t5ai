Bluetooth gatt server
=======================================

Project Overview
--------------------------

    This project is used for demonstrating bluetooth gatt server, show you the following operation:

    - how to config ble advertising pararmeters and datas, how to start the advertising.
    - how to make up a gatt service database and process the att read write request.

Hardware Requirements
----------------------------

    Beken development board.


Build
----------------------------

    Enter the following command in the Armino root directory to compile:

    > make bk72xx PROJECT=bluetooth/gatt_server

Connect UART
----------------------------

    UART0 is used for:

    - BKFIL to download images
    - Logging and Cli commands

Flash all-app.bin
----------------------------

    Flash Armino/build/gatt_server/bk72xx/all-app.bin to the board by the tool BKFIL.

Play and Output
----------------------------------
    Restart the board, the gatt_server will advertise automatically with the name ``doorbell_xxxxxx``, then you can use another board falshed with project gatt_client or other ble tools to connect with this board.
 
Initiate Log
***********************************
::

    [15:51:01.911]IN←◆ble:I(816):ble create new db
    ble:I(820):ble_env->start_hdl = 0x10
    ble:I(820):[gapm_profile_added_ind_handler] prf_task_id:0x6e,prf_task_nb:9,start_hdl:16,state:0x1
    ble:I(820):conidx:0x0,role:0x0,dest_id:0x3,src_id:0x8,param->status:0x0
    BLE-GATT:I(820):cd_ind:prf_id:10, status:0
    ble:I(820):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0x1b,status:0x0
    BLE-GATT:I(820):create gatt db success
    BLE-GATT:I(820):gatt_server_demo_init, dev_name:doorbell_7FCC5F, ret:15 
    BLE-GATT:I(822):adv data length :28 
    ble:I(828):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa0,status:0x0
    BLE-GATT:I(828):set adv paramters success
    ble:I(836):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa9,status:0x0
    BLE-GATT:I(836):set adv data success
    ble:I(842):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa4,status:0x0
    BLE-GATT:I(842):sart adv success
    BLE-GATT:I(842):gatt_server_demo_init success

Connected Log
***********************************

::

    [15:51:30.373]IN←◆tempd:I(29278):change config, interval=15, threshold=46, dist=0

    [15:51:45.504]IN←◆ble:I(44408):[gapc_connection_req_ind_handler]conidx:0,dest_id:0x3,con_peer_addr:63:2b:d8:b6:fd:50
    ble:I(44408):[gapc_connection_req_ind_handler]ble_slave conn_idx:0
    ble:I(44408):addr:50:fd:b6:d8:2b:63
    BLE-GATT:I(44408):c_ind:conn_idx:0, addr_type:1, peer_addr:63:2b:d8:b6:fd:50
    ble:I(44410):[gapm_activity_stopped_ind_handler]conidx:0

    [15:51:45.606]IN←◆ble:I(44510):gapc_le_pkt_size_ind_handler msgid:0xe2c,dest_id:0x3,src_id:0x7
    ble:I(44510):conidx:0,ble:I(44510):1max_rx_octets = 27
    ble:I(44510):1max_rx_time = 328
    ble:I(44510):1max_tx_octets = 251
    ble:I(44510):1max_tx_time = 2120

Work flow chart
----------------------------------
1. gatt server demo work flow chart:

.. figure:: ../../../../_static/gatt_server_flow_chart.png
    :align: center
    :alt: gatt server demo work flow chart
    :figclass: align-center

    Figure 1. gatt server demo work flow

Cli Command
----------------------------------
    | The project gatt_server supports the following commands through uart0:
    | ``ble_gatts help`` : get the list of commands.
    | ``ble_gatts notify`` : launch the att notify request after connected by other device.
    | ``ble_gatts adv_en 1`` : start the advertising, the advertising will stop once connected by other device.
    | ``ble_gatts adv_en 0`` : stop the advertising.

Command Log
***********************************
- ble_gatts notify

::

    [15:52:00.140]OUT→◇ble_gatts notify
    [15:52:00.145]IN←◆ble_gatts notify
    os:I(59050):create shell_handle, tcb=28066de8, stack=[28072a70-28073e70:5120], prio=5

    $BLE GATTS RSP:OK

    //gatt client demo project log
    [11:38:27.008]IN←◆BLE-GATT:I(3446536):CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x12, len:5 
    BLE-GATT:I(3446536):
    ==================
    BLE-GATT:I(3446536): 
    BLE-GATT:I(3446536):
    ==================
    BLE-GATT:I(3446536):0x00 0x00 0x00 0x00
    BLE-GATT:I(3446536):
    ==================

- ble_gatts adv_en 1

::

    [15:54:26.103]OUT→◇ble_gatts adv_en 1
    [15:54:26.109]IN←◆ble_gatts adv_en 1
    os:I(205014):create shell_handle, tcb=28066de8, stack=[28072a70-28073e70:5120], prio=5
    ble:I(205022):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa4,status:0x0

    BLE-GATT:I(205022):sart adv success
    BLE GATTS RSP:OK


Advertising and gatt service API
----------------------------------------
    please refer to the souce code in the flie ``/projects/bluetooth/gatt_server/main/gatt_server_demo.c``.

- setp1. register ble event callback function

::

    static void ble_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
    {
        s_at_cmd_status = param->status;
        switch (cmd)
        {
            case BLE_CREATE_ADV:
            case BLE_SET_ADV_DATA:
            case BLE_SET_RSP_DATA:
            case BLE_START_ADV:
            case BLE_STOP_ADV:
            case BLE_CREATE_SCAN:
            case BLE_START_SCAN:
            case BLE_STOP_SCAN:
            case BLE_INIT_CREATE:
            case BLE_INIT_START_CONN:
            case BLE_INIT_STOP_CONN:
            case BLE_CONN_DIS_CONN:
            case BLE_CONN_UPDATE_PARAM:
            case BLE_DELETE_ADV:
            case BLE_DELETE_SCAN:
            case BLE_CONN_READ_PHY:
            case BLE_CONN_SET_PHY:
            case BLE_CONN_UPDATE_MTU:
                if (gatt_sema != NULL)
                    rtos_set_semaphore( &gatt_sema );
                break;
            default:
                break;
        }

    }

    void ble_at_notice_cb(ble_notice_t notice, void *param)
    {
            switch (notice) 
            {
                //setp9 setp10
                case BLE_5_WRITE_EVENT: 
                //setp7
                case BLE_5_READ_EVENT:
                    //setp8
                    ble_read_req_t *r_req = (ble_read_req_t *)param;
                    bk_ble_read_response_value(r_req->conn_idx, sizeof(tmp), &tmp, r_req->prf_id, r_req->att_idx);
                    break;
                //setp6
                case BLE_5_CONNECT_EVENT:
                case BLE_5_DISCONNECT_EVENT:
                ....
                break;
            }
    }
    bk_ble_set_notice_cb(ble_gatts_notice_cb);


- step2. make up a gatt database
    please make up the database before start the advertising.

::

    #define GATTS_SERVICE_UUID              (0xFA00)
    #define GATTS_CHARA_PROPERTIES_UUID     (0xEA01)
    #define GATTS_CHARA_N1_UUID             (0xEA02)
    #define GATTS_CHARA_N2_UUID             (0xEA05)
    #define GATTS_CHARA_N3_UUID             (0xEA06)

    #define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    #define DECL_CHARACTERISTIC_128      {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    #define DESC_CLIENT_CHAR_CFG_128     {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

    //database
    static ble_attm_desc_t gatts_service_db[GATTS_IDX_NB] = {
        //  Service Declaration
        [GATTS_IDX_SVC]        = {DECL_PRIMARY_SERVICE_128, GATTS_SERVICE_UUID >> 8}, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},

        [GATTS_IDX_CHAR_DECL]  = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
        // Characteristic Value
        [GATTS_IDX_CHAR_VALUE] = {{GATTS_CHARA_PROPERTIES_UUID & 0xFF, GATTS_CHARA_PROPERTIES_UUID >> 8}, BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
        //Client Characteristic Configuration Descriptor
        [GATTS_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},

        //opreation
        [GATTS_IDX_CHAR_N1_DECL]  = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
        [GATTS_IDX_CHAR_N1_VALUE] = {{GATTS_CHARA_N1_UUID & 0xFF, GATTS_CHARA_N1_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

        //s_v
        [GATTS_IDX_CHAR_N2_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
        [GATTS_IDX_CHAR_N2_VALUE]   = {{GATTS_CHARA_N2_UUID & 0xFF, GATTS_CHARA_N2_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

        //p_v
        [GATTS_IDX_CHAR_N3_DECL]    = {DECL_CHARACTERISTIC_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
        [GATTS_IDX_CHAR_N3_VALUE]   = {{GATTS_CHARA_N3_UUID & 0xFF, GATTS_CHARA_N3_UUID >> 8, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
    };

    //create the service database
    ble_db_cfg.att_db = (ble_attm_desc_t *)gatts_service_db;
    ble_db_cfg.att_db_nb = GATTS_IDX_NB;
    ble_db_cfg.prf_task_id = PRF_TASK_ID_GATTS;
    ble_db_cfg.start_hdl = 0;
    ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
    ble_db_cfg.uuid[0] = GATTS_SERVICE_UUID & 0xFF;
    ble_db_cfg.uuid[1] = GATTS_SERVICE_UUID >> 8;

    ret = bk_ble_create_db(&ble_db_cfg);

    if (ret == BK_OK)
    {
        LOGI("create gatt db success\n");
    }

- setp3 step4 setp5. start a advertising

::

    ble_adv_param_t adv_param;
    int actv_idx = 0;
    bt_err_t ret = BK_FAIL;

    /* set adv paramters */
    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = ADV_ALL_CHNLS;
    adv_param.adv_intv_min = BLE_GATTS_ADV_INTERVAL_MIN;
    adv_param.adv_intv_max = BLE_GATTS_ADV_INTERVAL_MAX;
    adv_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_OR_STATIC_ADDR;
    adv_param.adv_type = ADV_TYPE_LEGACY;
    adv_param.adv_prop = ADV_PROP_CONNECTABLE_BIT | ADV_PROP_SCANNABLE_BIT;
    adv_param.prim_phy = INIT_PHY_TYPE_LE_1M;
    adv_param.second_phy = INIT_PHY_TYPE_LE_1M;

    ret = bk_ble_create_advertising(actv_idx, &adv_param, ble_cmd_cb);
    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGE("config adv paramters failed %d\n", ret);
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, AT_SYNC_CMD_TIMEOUT_MS);

    /* set adv data */
    uint8_t adv_data[ADV_MAX_SIZE] = {0};
    uint8_t adv_index = 0;
    uint8_t len_index = 0;
    uint8_t mac[6];
    int ret;

    /* flags */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_FLAGS;
    adv_data[adv_index++] = 0x06;
    adv_data[len_index] = 2;

    /* local name */
    bk_bluetooth_get_address(mac);

    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_LOCAL_NAME;

    ret = sprintf((char *)&adv_data[adv_index], "%s_%02X%02X%02X", ADV_NAME_HEAD, mac[0], mac[1], mac[2]);

    bk_ble_appm_set_dev_name(ret, &adv_data[adv_index]);
    BLEGATTS_LOGI("%s, dev_name:%s, ret:%d \n", __func__, (char *)&adv_data[adv_index], ret);
    adv_index += ret;
    adv_data[len_index] = ret + 1;

    /* 16bit uuid */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_SERVICE_DATA;
    adv_data[adv_index++] = GATTS_UUID & 0xFF;
    adv_data[adv_index++] = GATTS_UUID >> 8;
    adv_data[len_index] = 3;

    /* manufacturer */
    len_index = adv_index;
    adv_data[adv_index++] = 0x00;
    adv_data[adv_index++] = ADV_TYPE_MANUFACTURER_SPECIFIC;
    adv_data[adv_index++] = BEKEN_COMPANY_ID & 0xFF;
    adv_data[adv_index++] = BEKEN_COMPANY_ID >> 8;
    adv_data[len_index] = 3;

    ret = bk_ble_set_adv_data(actv_idx, adv_data, adv_len, ble_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGE("set adv data failed %d\n", ret);
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, AT_SYNC_CMD_TIMEOUT_MS);

    /* sart adv */
    ret = bk_ble_start_advertising(actv_idx, 0, ble_cmd_cb);

    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGE("start adv failed %d\n", ret);
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, AT_SYNC_CMD_TIMEOUT_MS);



- Detailed api reference please go to ``api-reference/bluetooth/ble.html``
