Bluetooth gatt client
=======================================

Project Overview
--------------------------

    This project is used for demonstrating bluetooth gatt client, show you the following operation:

    -  how to config ble scan pararmeters and start or stop it, how to connect to a advertising device.
    -  how to launch att read write request to remote gatt service.

Hardware Requirements
----------------------------

    Beken development board.


Build
----------------------------

    Enter the following command in the Armino root directory to compile:

    > make bk72xx PROJECT=bluetooth/gatt_client

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

Initiate Log
***********************************

::

    [10:41:01.307]收←◆ble:I(826):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xb3,status:0x0
    ble:I(826):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa1,status:0x0
    BLE-GATT:I(826):gatt_client_demo_init success

Work flow chart
----------------------------------
1. gatt client demo work flow chart:

.. figure:: ../../../../_static/gatt_client_flow_chart.png
    :align: center
    :alt: gatt client demo work flow chart
    :figclass: align-center

    Figure 1. gatt client demo work flow


Cli Command
----------------------------------

    | The project gatt_server supports the following commands through uart0:
    | ``ble_gattc help`` : get the list of commands.
    | ``ble_gattc scan 1`` : start scan. 
    | ``ble_gattc scan 0`` : stop scan.
    | ``ble_gattc conn xx:xx:xx:xx:xx:xx`` : connect to the device, the paramters is the remote's address.
    | ``ble_gattc disconn`` : disconnect to the connected device.
    | ``ble_gattc notifyindcate_en 1 [desc_handle]`` : enable the notify or indicate, the parameter [desc_handle] is the characteristic descriptor's handle.
    | ``ble_gattc notifyindcate_en 0 [desc_handle]`` : disable the notify or indicate, the parameter [desc_handle] is the characteristic descriptor's handle.
    | ``ble_gattc write [val_handle] [data]`` : att write request, the parameter [val_handle] is the characteristic value handle, and the [data] is the datas to be sent in the write request.
    | ``ble_gattc read [val_handle]`` : att read request, the parameter [val_handle] is the characteristic value handle.


Command Log
***********************************

- ble_gattc scan 1
    The scan paramters are initiated in the init code, the scan results will come up in the callback function.

::

    [11:02:21.684]OUT→◇ble_gattc scan 1
    [11:02:21.689]IN←◆ble_gattc scan 1
    os:I(1281212):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5
    ble:I(1281220):[gapm_cmp_evt_handler]actv_idx:0 conidx:0,operation:0xa4,status:0x0

    BLE GATTC RSP:OK

    $BLE-GATT:I(1281228):ADV_IND, addr_type:0, adv_addr:39:19:4f:8c:47:c8
    BLE-GATT:I(1281232):ADV_IND, addr_type:1, adv_addr:56:1f:90:46:44:49
    BLE-GATT:I(1281234):ADV_IND, addr_type:1, adv_addr:bc:e7:54:46:db:65
    BLE-GATT:I(1281236):ADV_IND, addr_type:0, adv_addr:49:f4:57:8c:47:c8
    BLE-GATT:I(1281238):ADV_NONCONN_IND, addr_type:1, adv_addr:09:66:4f:8f:63:56

- ble_gattc conn xx:xx:xx:xx:xx:xx
    The advertising is launched on another bk7236 board, use this command to connect to it.
    As the log show, the demo project prints all the service and characteristic's information defined in the remote demo project gatt_server service database, 
    including uuid,val_handle and desc_handle can be used as pararmeters in the nex read/write cli command.

::

    [11:07:38.977]OUT→◇ble_gattc conn c8:47:8c:5f:cc:7f
    [11:07:38.983]IN←◆ble_gattc conn c8:47:8c:5f:cc:7f
    os:I(1598508):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5
    ble:I(1598516):[gapm_activity_created_ind_handler]conidx:8,idx:0,p_param->actv_idx:1
    ble:I(1598516):[gapm_cmp_evt_handler]actv_idx:0 conidx:8,operation:0xa2,status:0x0
    ble:I(1598516):Cmd[20]operation[32]BLE_INIT_CREATE

    ble:I(1598524):[gapm_cmp_evt_handler]actv_idx:0 conidx:8,operation:0xa4,status:0x0
    ble:I(1598524):[gapm_cmp_evt_handler]actv_idx:0,init_state:APP_INIT_STATE_CONECTTING
    ble:I(1598524):Cmd[21]operation[24]BLE_INIT_START_CONN
    BLE GATTC RSP:OK

    $ble:I(1598584):[gapc_connection_req_ind_handler]conidx:0,dest_id:0x803,con_peer_addr:7f:cc:5f:8c:47:c8
    ble:I(1598584):[gapc_connection_req_ind_handler]ble_master conn_idx:0
    ble:I(1598584):addr:c8:47:8c:5f:cc:7f
    ble:I(1598584):KERNEL_IDX:0,conn_idx:0
    BLE-GATT:I(1598586):BLE_5_INIT_CONNECT_EVENT:conn_idx:0, addr_type:0, peer_addr:7f:cc:5f:8c:47:c8
    ble:I(1598586):[gapm_activity_stopped_ind_handler]conidx:8
    ble:I(1598588):gapc_le_pkt_size_ind_handler msgid:0xe2c,dest_id:0x803,src_id:0x7
    ble:I(1598588):conidx:0,ble:I(1598588):1max_rx_octets = 251
    ble:I(1598588):1max_rx_time = 2120
    ble:I(1598588):1max_tx_octets = 251
    ble:I(1598588):1max_tx_time = 2120

    [11:07:39.317]IN←◆ble:I(1598840):[app_init_con_set_mtu_timer_handler]conidx:0

    [11:07:39.361]IN←◆ble:I(1598884):[app_init_con_sdp_up_timer_handler]conidx:0

    [11:07:39.462]IN←◆ble:I(1598986):[app_init_con_sdp_wait_timer_handler]conidx:0
    ble:I(1598996):[app_init_con_sdp_start_timer_handler]conidx:0
    ble:I(1598996):[sdp_discover_all_service]conidx:0,conhdl:0

    [11:07:39.627]IN←◆BLE-GATT:I(1599150):==>Get GATT Service UUID:0x1800, start_handle:0x01
    BLE-GATT:I(1599150):==>Get GATT Characteristic UUID:0x2A00, cha_handle:0x02, val_handle:0x03, property:0x02
    BLE-GATT:I(1599150):==>Get GATT Characteristic UUID:0x2A01, cha_handle:0x04, val_handle:0x05, property:0x02
    BLE-GATT:I(1599150):==>Get GATT Characteristic UUID:0x2AC9, cha_handle:0x06, val_handle:0x07, property:0x02
    ble:I(1599152):[sdp_init]app_task:0x803 
    ble:I(1599152):env:0x28050a3c,28050e2c,sdp_env:0x28050e2c
    ble:I(1599152):[sdp_task_init]conidx:0,srv_env:0x28050e2c,task_id:140
    ble:I(1599152):[sdp_init]env->task:9,conhdl:0,cur_state:0
    ble:I(1599152):[gapm_profile_added_ind_handler] prf_task_id:0x8c,prf_task_nb:9,start_hdl:1,state:0x0
    ble:I(1599152):conidx:0x8,role:0x1,dest_id:0x803,src_id:0x8,param->status:0x0
    ble:I(1599152):[gapm_cmp_evt_handler]actv_idx:0 conidx:8,operation:0x1b,status:0x0

    [11:07:39.740]IN←◆BLE-GATT:I(1599262):==>Get GATT Service UUID:0x1801, start_handle:0x08
    BLE-GATT:I(1599262):==>Get GATT Characteristic UUID:0x2A05, cha_handle:0x09, val_handle:0x0A, property:0x20
    BLE-GATT:I(1599262):==>Get GATT Characteristic UUID:0x2B29, cha_handle:0x0C, val_handle:0x0D, property:0x0a
    BLE-GATT:I(1599262):==>Get GATT Characteristic UUID:0x2B2A, cha_handle:0x0E, val_handle:0x0F, property:0x02
    BLE-GATT:I(1599262):==>Get GATT Characteristic Description UUID:0x2902, desc_handle:0x0B, char_index:0 
    ble:I(1599264):[sdp_init]app_task:0x803 
    ble:I(1599264):env:0x28050a54,28050e60,sdp_env:0x28050e60
    ble:I(1599264):[sdp_task_init]conidx:0,srv_env:0x28050e60,task_id:140
    ble:I(1599264):[sdp_init]env->task:10,conhdl:0,cur_state:0
    ble:I(1599264):[gapm_profile_added_ind_handler] prf_task_id:0x8d,prf_task_nb:10,start_hdl:8,state:0x0
    ble:I(1599264):conidx:0x8,role:0x1,dest_id:0x803,src_id:0x8,param->status:0x0
    ble:I(1599264):[gapm_cmp_evt_handler]actv_idx:0 conidx:8,operation:0x1b,status:0x0

    [11:07:39.875]IN←◆BLE-GATT:I(1599398):==>Get GATT Service UUID:0xFA00, start_handle:0x10
    BLE-GATT:I(1599398):==>Get GATT Characteristic UUID:0xEA01, cha_handle:0x11, val_handle:0x12, property:0x10
    BLE-GATT:I(1599398):==>Get GATT Characteristic UUID:0xEA02, cha_handle:0x14, val_handle:0x15, property:0x08
    BLE-GATT:I(1599398):==>Get GATT Characteristic UUID:0xEA05, cha_handle:0x16, val_handle:0x17, property:0x0a
    BLE-GATT:I(1599398):==>Get GATT Characteristic UUID:0xEA06, cha_handle:0x18, val_handle:0x19, property:0x0a
    BLE-GATT:I(1599400):==>Get GATT Characteristic Description UUID:0x2902, desc_handle:0x13, char_index:0 
    ble:I(1599400):
    APPC_SERVICE_CONNECTED (8,0)
    BLE-GATT:I(1599400):=============
    ble:I(1599400):[sdp_init]app_task:0x803 
    ble:I(1599400):env:0x28050a6c,28050e94,sdp_env:0x28050e94
    ble:I(1599400):[sdp_task_init]conidx:0,srv_env:0x28050e94,task_id:140
    ble:I(1599400):[sdp_init]env->task:11,conhdl:0,cur_state:0
    ble:I(1599400):[gapm_profile_added_ind_handler] prf_task_id:0x8e,prf_task_nb:11,start_hdl:16,state:0x3
    ble:I(1599400):conidx:0x8,role:0x1,dest_id:0x803,src_id:0x8,param->status:0x0
    ble:I(1599400):[gapm_cmp_evt_handler]actv_idx:0 conidx:8,operation:0x1b,status:0x0
    BLE-GATT:I(1599420):gattc_notice_cb m_ind:conn_idx:0, mtu_size:255
    ble:I(1599420):up_c_idx:0

- ble_gattc disconn:

::

    [11:20:15.902]OUT→◇ble_gattc disconn
    [11:20:15.907]IN←◆ble_gattc disconn
    os:I(2355434):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5
    ble:I(2355444):[gapc_disconnect_ind_handler]conidx:0,conhdl:1,reason:0x16

    $BLE-GATT:I(2355444):BLE_5_INIT_DISCONNECT_EVENT:conn_idx:0,reason:22
    ble:I(2355444):gapc_cmp_evt_handler conidx:5,operation:0x1,status:0
    BLE-GATT:I(2355444):BLE_CONN_DIS_CONN
    BLE GATTC RSP:OK

- ble_gattc notifyindcate_en 1 [desc_handle]
    | ``==>Get GATT Characteristic Description UUID:0x2902, desc_handle:0x13, char_index:0`` 
    | As the above log show, the descriptor handle is 0x13, so we use 13 as the parameter in this command.

::

    [11:23:49.409]OUT→◇ble_gattc notifyindcate_en 1 13
    [11:23:49.416]IN←◆ble_gattc notifyindcate_en 1 13
    os:I(2568944):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5

    $BLE GATTC RSP:OK
    $BLE-GATT:I(2568968):CHARAC_WRITE_DONE, handle:0x00, len:0 


    //remote device Log:
    [16:02:49.819]IN←◆BLE-GATT:I(58988):write_cb:conn_idx:0, prf_id:10, att_idx:3, len:2, data[0]:0x01
    BLE-GATT:I(58988):write notify: 01 00, length: 2

- ble_gattc notifyindcate_en 0 [desc_handle]

::

    [11:30:35.366]OUT→◇ble_gattc notifyindcate_en 0 13
    [11:30:35.372]IN←◆ble_gattc notifyindcate_en 0 13
    os:I(2974902):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5

    $BLE GATTC RSP:OK

    $BLE-GATT:I(2974924):CHARAC_WRITE_DONE, handle:0x00, len:0 


    //remote device Log:
    [16:02:49.819]IN←◆BLE-GATT:I(58988):write_cb:conn_idx:0, prf_id:10, att_idx:3, len:2, data[0]:0x01
    BLE-GATT:I(58988):write notify: 00 00, length: 2

- ble_gattc write [val_handle] [data]
    | ``==>Get GATT Characteristic UUID:0xEA05, cha_handle:0x16, val_handle:0x17, property:0x0a``
    | As the abvoe information show in the connect log, the value handle is 0x17 can be used as parameter in this command.

::

    [11:31:39.560]OUT→◇ble_gattc write 17 ssid_ab
    [11:31:39.565]IN←◆ble_gattc write 17 ssid_ab
    os:I(3039094):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5

    $BLE GATTC RSP:OK

    $BLE-GATT:I(3039118):CHARAC_WRITE_DONE, handle:0x00, len:0 


    //remote device Log:
    [16:03:53.179]IN←◆BLE-GATT:I(122348):write_cb:conn_idx:0, prf_id:10, att_idx:7, len:7, data[0]:0x73
    BLE-GATT:I(122348):write N2: ssid_ab, length: 7

- ble_gattc read [val_handle]:

::

    [11:35:48.307]OUT→◇ble_gattc read 17
    [11:35:48.312]IN←◆ble_gattc read 17
    os:I(3287840):create shell_handle, tcb=28066cf8, stack=[28072980-28073d80:5120], prio=5

    $BLE GATTC RSP:OK

    $BLE-GATT:I(3287866):CHARAC_READ|CHARAC_READ_DONE, handle:0x17, len:7 
    BLE-GATT:I(3287866):
    ==================
    BLE-GATT:I(3287866):ssid_ab 
    BLE-GATT:I(3287866):
    ==================
    BLE-GATT:I(3287866):0x73 0x73 0x69 0x64
    BLE-GATT:I(3287866):
    ==================
    BLE-GATT:I(3287866):CHARAC_READ|CHARAC_READ_DONE, handle:0x00, len:0 


    //remote device Log:
    [16:04:22.046]IN←◆BLE-GATT:I(151216):read_cb:conn_idx:0, prf_id:10, att_idx:7
    BLE-GATT:I(151216):read N2: ssid_ab, length: 7

- remote device notify 
    We use gatt_server demo project board to launch a notify event after enable the notify characteristic.

::

    //remote device launch notify
    [11:38:26.991]OUT→◇ble_gatts notify
    [11:38:26.997]IN←◆ble_gatts notify
    os:I(1850370):create shell_handle, tcb=28066d28, stack=[28072980-28073d80:5120], prio=5

    BLE GATTS RSP:OK

    $
    [11:38:51.815]OUT→◇ble_gatts notify
    [11:38:51.819]IN←◆ble_gatts notify
    os:I(1875194):create shell_handle, tcb=28066d28, stack=[28072980-28073d80:5120], prio=5

    BLE GATTS RSP:OK

    $
    [11:38:52.331]OUT→◇ble_gatts notify
    [11:38:52.336]IN←◆ble_gatts notify
    os:I(1875712):create shell_handle, tcb=28066d28, stack=[28072980-28073d80:5120], prio=5

    BLE GATTS RSP:OK

    //we receive the event
    [11:38:27.008]IN←◆BLE-GATT:I(3446536):CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x12, len:5 
    BLE-GATT:I(3446536):
    ==================
    BLE-GATT:I(3446536): 
    BLE-GATT:I(3446536):
    ==================
    BLE-GATT:I(3446536):0x00 0x00 0x00 0x00
    BLE-GATT:I(3446536):
    ==================

    [11:38:51.826]IN←◆BLE-GATT:I(3471354):CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x12, len:5 
    BLE-GATT:I(3471354):
    ==================
    BLE-GATT:I(3471
    [11:38:51.854]IN←◆354): 
    BLE-GATT:I(3471354):
    ==================
    BLE-GATT:I(3471354):0x01 0x00 0x00 0x00
    BLE-GATT:I(3471354):
    ==================

    [11:38:52.343]IN←◆BLE-GATT:I(3471872):CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x12, len:5 
    BLE-GATT:I(3471872):
    ==================
    BLE-GATT:I(3471
    [11:38:52.371]IN←◆872): 
    BLE-GATT:I(3471872):
    ==================
    BLE-GATT:I(3471872):0x02 0x00 0x00 0x00
    BLE-GATT:I(3471872):


Scan and Connect Disconnect API
----------------------------------------
    The source code is in the file ``/projects/bluetooth/gatt_client/main/gatt_client_demo.c``

- setp1 setp2 setp3. register the callback functions

::

    static void ble_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
    {
        BLEGATTC_LOGI("--------%s %d\n", __func__, cmd);
        gatt_cmd_status = param->status;
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
            case BLE_CREATE_PERIODIC:
            case BLE_START_PERIODIC:
            case BLE_STOP_PERIODIC:
            case BLE_DELETE_PERIODIC:
            case BLE_SET_LOCAL_NAME:
            case BLE_GET_LOCAL_NAME:
            case BLE_READ_LOCAL_ADDR:
            case BLE_SET_RANDOM_ADDR:
                if (gatt_sema != NULL)
                    rtos_set_semaphore( &gatt_sema );
                break;
            default:
                break;
        }
    }

    static void gattc_notice_cb(ble_notice_t notice, void *param)
    {
        switch (notice) 
        {
        case BLE_5_WRITE_EVENT: 
            break;
        case BLE_5_READ_EVENT: 
            break;
        //setp6
        case BLE_5_REPORT_ADV:
        break;
        case BLE_5_MTU_CHANGE: 
        case BLE_5_CONNECT_EVENT: 
        case BLE_5_DISCONNECT_EVENT: 
        //setp8
        case BLE_5_INIT_CONNECT_EVENT: 
        case BLE_5_INIT_DISCONNECT_EVENT: 
        case BLE_5_INIT_CONNECT_FAILED_EVENT:
        case BLE_5_SDP_REGISTER_FAILED:
        case BLE_5_READ_PHY_EVENT: 
        case BLE_5_CONN_UPDATA_EVENT:
        case BLE_5_PERIODIC_SYNC_CMPL_EVENT:
        case BLE_5_CONN_UPD_PAR_ASK:
        case BLE_5_PAIRING_REQ:
        case BLE_5_PARING_PASSKEY_REQ:
        case BLE_5_ENCRYPT_EVENT:
        case BLE_5_PAIRING_SUCCEED:
        case BLE_5_PAIRING_FAILED:
        case BLE_5_GAP_CMD_CMP_EVENT:
        {
            ble_cmd_cmp_evt_t *event = (ble_cmd_cmp_evt_t *)param;
            switch(event->cmd) {
            case BLE_CONN_DIS_CONN:
                BLEGATTC_LOGI("BLE_CONN_DIS_CONN\r\n");
                if (gatt_sema != NULL)
                    rtos_set_semaphore( &gatt_sema );
                gatt_cmd_status = event->status;
                break;
            case BLE_CONN_UPDATE_PARAM:
            case BLE_CONN_SET_PHY:
            case BLE_CONN_READ_PHY:
            case BLE_CONN_UPDATE_MTU:
            case BLE_SET_MAX_MTU:
            {
                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
    }

    //setp9
    //gatt callback function
    static void gattc_sdp_comm_callback(MASTER_COMMON_TYPE type,uint8 conidx,void *param)
    {
        uint16_t uuid = 0xFF;
        if(MST_TYPE_SVR_UUID == type)
        {
            struct ble_sdp_svc_ind *srv_ind = (struct ble_sdp_svc_ind*)param;
            if(srv_ind->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(srv_ind->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
                }else
                {
                    uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
                }
            }else if(srv_ind->uuid_len == 2)
            {
                uuid = srv_ind->uuid[1]<<8 | srv_ind->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Service UUID:0x%04X, start_handle:0x%02X\n", uuid, srv_ind->start_hdl);
            }
        }else if (MST_TYPE_ATT_UUID == type)
        {
            struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf*)param;
            if(char_inf->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(char_inf->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
                }else
                {
                    uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
                }
            }else if(char_inf->uuid_len == 2)
            {
                uuid = char_inf->uuid[1]<<8 | char_inf->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Characteristic UUID:0x%04X, cha_handle:0x%02X, val_handle:0x%02X, property:0x%02x\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop);
            }
        }else if(MST_TYPE_ATT_DESC == type)
        {
            struct ble_sdp_char_desc_inf *desc_inf = (struct ble_sdp_char_desc_inf*)param;
            if(desc_inf->uuid_len == 16)
            {
                if(ble_convert_128b_2_16b_uuid(desc_inf->uuid, &uuid) == 0)
                {
                    BLEGATTC_LOGI("====>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
                }else
                {
                    uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
                    BLEGATTC_LOGI("Custom UUID\n");
                    BLEGATTC_LOGI("===>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0X%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
                }
            }else if(desc_inf->uuid_len == 2)
            {
                uuid = desc_inf->uuid[1]<<8 | desc_inf->uuid[0];
                BLEGATTC_LOGI("==>Get GATT Characteristic Description UUID:0x%04X, desc_handle:0x%02X, char_index:%d \n", uuid, desc_inf->desc_hdl, desc_inf->char_code);
            }
        }else if (MST_TYPE_SDP_END == type)
        {
            BLEGATTC_LOGI("=============\r\n");
            bk_ble_gatt_mtu_change(conidx);
        }
        else if(type == MST_TYPE_UPP_ASK)
        {
            struct mst_comm_updata_para *tmp = (typeof(tmp))param;
            BLEGATTC_LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
            tmp->is_agree = 1;
        }

    }
    //setp11/13/14
    static void gattc_sdp_charac_callback(CHAR_TYPE type,uint8 conidx,uint16_t hdl,uint16_t len,uint8 *data)
    {
    //    BLEGATTC_LOGI("%s type:%d len:%d, data:%s\n",__func__, type, len, data);
        if (CHARAC_NOTIFY == type || CHARAC_INDICATE == type)
        {
            BLEGATTC_LOGI("CHARAC_NOTIFY|CHARAC_INDICATE, handle:0x%02x, len:%d \n", hdl, len);
        }
        else if (CHARAC_WRITE_DONE == type)
        {
            BLEGATTC_LOGI("CHARAC_WRITE_DONE, handle:0x%02x, len:%d \n", hdl, len);
        }
        else if(CHARAC_READ == type || CHARAC_READ_DONE==type)
        {
            BLEGATTC_LOGI("CHARAC_READ|CHARAC_READ_DONE, handle:0x%02x, len:%d \n", hdl, len);
        }

        if(len)
        {
            BLEGATTC_LOGI("\n==================\n");
            char s[100] = {0};
            os_memcpy(s, data, len);
            BLEGATTC_LOGI("%s \n", s);
            BLEGATTC_LOGI("\n==================\n");
            if(len>=4)
            BLEGATTC_LOGI("0x%02x 0x%02x 0x%02x 0x%02x\n", data[0],data[1],data[2],data[3]);
            BLEGATTC_LOGI("\n==================\n");
        }
    }
    bk_ble_set_notice_cb(gattc_notice_cb);
    bk_ble_register_app_sdp_common_callback(gattc_sdp_comm_callback);
    bk_ble_register_app_sdp_charac_callback(gattc_sdp_charac_callback);

    
- step4 step5. start scan

::

    uint8_t actv_idx = 0;
    ble_err_t ret = BK_FAIL;

    ret = rtos_init_semaphore(&gatt_sema, 1);
    if(ret != BK_OK){
        BLEGATTC_LOGE("%s ,init sema error!\n", __func__);
        return;
    }

    //config the scan paramters
    ble_scan_param_t scan_param;
    os_memset(&scan_param, 0, sizeof(ble_scan_param_t));
    scan_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_OR_STATIC_ADDR;
    scan_param.scan_phy = INIT_PHY_TYPE_LE_1M;
    scan_param.scan_intv = GATTC_SCAN_PARAM_INTERVAL;
    scan_param.scan_wd = GATTC_SCAN_PARAM_WINDOW;
    scan_param.scan_type = PASSIVE_SCANNING;
    //set 4
    ret = bk_ble_create_scaning(actv_idx, &scan_param, ble_cmd_cb);

    if(ret != BK_OK){
        BLEGATTC_LOGE("%s ,create scan error!\n", __func__);
        return;
    }
    ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("%s ,create scan timeout!\n", __func__);
        return;
    }
    //setp5 start scan
    ret = bk_ble_start_scaning_ex(actv_idx, 0, 0, 10, ble_cmd_cb);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble set scan enable fail \n");
        goto error;
    }
    if(gatt_sema != NULL)
    {
        ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
        if(ret != BK_OK)
        {
            BLEGATTC_LOGE("ble set scan enable timeout \n");
            goto error;
        }
    }


- setp7. ble create connection

::

    //setp7
    //remote address
    bd_addr_t addr;
    uint8_t actv_idx = 0;

    ble_conn_param_t conn_param;
    os_memset(&conn_param, 0, sizeof(ble_conn_param_t));
    conn_param.intv_max = conn_param.intv_min = GATTC_CONN_PARA_INTERVAL;
    conn_param.con_latency = GATTC_CONN_PARA_LATENCY;
    conn_param.sup_to = GATTC_CONN_PARA_SUPERVISION_TIMEOUT;
    conn_param.init_phys = INIT_PHY_TYPE_LE_1M | INIT_PHY_TYPE_LE_2M;
    actv_idx = bk_ble_get_idle_conn_idx_handle();
    if (actv_idx == UNKNOW_ACT_IDX)
    {
        BLEGATTC_LOGE("ble conn fail, no resource \n");
        goto error;
    }
    //set connect parameter
    ret = bk_ble_create_init(actv_idx, &conn_param, ble_cmd_cb);
    if (ret != BK_OK)
    {
        BLEGATTC_LOGE("ble create init fail \n");
        goto error;
    }
    if(gatt_sema == NULL)
    {
        BLEGATTC_LOGE("ble conn fail, gatt_sema need init \n");
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble create init timeout \n");
        goto error;
    }
    if(gatt_cmd_status != BK_OK)
    {
        BLEGATTC_LOGE("ble create init fail, ret_sta:%d \n", gatt_cmd_status);
        goto error;
    }
    //set remote device address type
    ret = bk_ble_init_set_connect_dev_addr(actv_idx, &addr, OWN_ADDR_TYPE_PUBLIC_OR_STATIC_ADDR);
    if (ret != BK_OK)
    {
        BLEGATTC_LOGE("ble set init fail \n");
        goto error;
    }
    //connect 
    ret = bk_ble_init_start_conn(actv_idx, ble_cmd_cb);
    if (ret != BK_OK)
    {
        BLEGATTC_LOGE("ble start init fail \n");
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, BLE_SYNC_CMD_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble start init timeout \n");
        goto error;
    }

- step10-14. att read write operation

::

    // notify enable and disable
    if(en)
    {
        ret = bk_ble_gatt_write_ccc(gatt_conn_ind, char_handle, 1);
    }else
    {
        ret = bk_ble_gatt_write_ccc(gatt_conn_ind, char_handle, 0);
    }
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble notify|indcate en fail :%d\n", ret);
        goto error;
    }

    //setp10
    //read
    uint16_t char_handle = 0x17;
    ret = bk_ble_att_read(gatt_conn_ind, char_handle);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble read att :%d\n", ret);
        goto error;
    }

    //setp12
    //write
    uint16_t char_handle = 0x17;
    char *data = "test";
    uint8_t len = os_strlen(data);
    ret = bk_ble_gatt_write_value(gatt_conn_ind, char_handle, len, (uint8_t *)data);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble read att :%d\n", ret);
        goto error;
    }

- disconnect

::

    ret = bk_ble_disconnect(gatt_conn_ind);
    if (ret != BK_OK)
    {
        BLEGATTC_LOGE("ble disconn fail :%d\n", ret);
        goto error;
    }
    if(gatt_sema == NULL)
    {
        BLEGATTC_LOGE("ble disconn fail, gatt_sema need init \n");
        goto error;
    }
    ret = rtos_get_semaphore(&gatt_sema, BLE_DISCON_CMD_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        BLEGATTC_LOGE("ble disconn timeout \n");
        goto error;
    }


- Detailed api reference please go to ``api-reference/bluetooth/ble.html``