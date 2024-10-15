#include "cli.h"

#include "dm_gatts.h"
#include "dm_gattc.h"


static void ble_gatt_demo_usage(void)
{
    CLI_LOGI("Usage:\n"
             "ble_gatt_demo gatts init\n"
             "ble_gatt_demo gattc init\n"
             "ble_gatt_demo gattc connect <xx:xx:xx:xx:xx:xx>\n"
             "ble_gatt_demo passkey <key>\n"
             "ble_gatt_demo security_method <iocap> <authen req>\n"
            );

    return;
}

static void cmd_ble_gatt_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    int ret = 0;

    if (argc == 1)
    {
        goto __usage;
    }
    else if (os_strcmp(argv[1], "-h") == 0)
    {
        goto __usage;
    }
    else if (os_strcmp(argv[1], "gatts") == 0 && argc >= 3)
    {
        if (os_strcmp(argv[2], "init") == 0)
        {
            dm_gatt_main();
            dm_gatts_main();
        }
        else if (os_strcmp(argv[2], "disconnect") == 0 && argc >= 4)
        {
            uint32_t mac[6] = {0};
            uint8_t mac_final[6] = {0};

            //sscanf bug: cant detect %hhx
            ret = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                         mac + 5,
                         mac + 4,
                         mac + 3,
                         mac + 2,
                         mac + 1,
                         mac);

            if (ret != 6)
            {
                CLI_LOGE("%s addr err %d\n", __func__, ret);
                goto __usage;
            }

            for (int i = 0; i < sizeof(mac_final); ++i)
            {
                mac_final[i] = (uint8_t)mac[i];
            }

            ret = dm_gatts_disconnect(mac_final);
        }
        else if (!os_strcmp(argv[2], "enable_service") && argc >= 5)
        {
            uint32_t service = 0;
            uint8_t enable = 0;

            ret = sscanf(argv[3], "%hhd", &enable);

            if (ret != 1)
            {
                CLI_LOGE("%s enable param err %d\n", __func__, ret);
                goto __usage;
            }

            ret = sscanf(argv[4], "%d", &service);

            if (ret != 1)
            {
                CLI_LOGE("%s service param err %d\n", __func__, ret);
                goto __usage;
            }

            ret = dm_gatts_enable_service(service, enable);
        }
        else if (os_strcmp(argv[2], "start_adv") == 0)
        {
            dm_gatts_start_adv();
        }
        else
        {
            goto __usage;
        }
    }
    else if (os_strcmp(argv[1], "gattc") == 0 && argc >= 3)
    {
        if (os_strcmp(argv[2], "init") == 0)
        {
            dm_gatt_main();
            dm_gattc_main();
        }
        else if (os_strcmp(argv[2], "connect") == 0 && argc >= 4)
        {
            uint32_t mac[6] = {0};
            uint8_t mac_final[6] = {0};

            //sscanf bug: cant detect %hhx
            ret = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                         mac + 5,
                         mac + 4,
                         mac + 3,
                         mac + 2,
                         mac + 1,
                         mac);

            if (ret != 6)
            {
                CLI_LOGE("%s addr err %d\n", __func__, ret);
                goto __usage;
            }

            for (int i = 0; i < sizeof(mac_final); ++i)
            {
                mac_final[i] = (uint8_t)mac[i];
            }

            ret = dm_gattc_connect(mac_final);
        }
        else if (os_strcmp(argv[2], "disconnect") == 0 && argc >= 4)
        {
            uint32_t mac[6] = {0};
            uint8_t mac_final[6] = {0};

            //sscanf bug: cant detect %hhx
            ret = sscanf(argv[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                         mac + 5,
                         mac + 4,
                         mac + 3,
                         mac + 2,
                         mac + 1,
                         mac);

            if (ret != 6)
            {
                CLI_LOGE("%s addr err %d\n", __func__, ret);
                goto __usage;
            }

            for (int i = 0; i < sizeof(mac_final); ++i)
            {
                mac_final[i] = (uint8_t)mac[i];
            }

            ret = dm_gattc_disconnect(mac_final);
        }
        else if (os_strcmp(argv[2], "discover") == 0 && argc >= 4)
        {
            uint16_t conn_id = 0;

            ret = sscanf(argv[3], "%hu", &conn_id);

            if (ret != 1)
            {
                CLI_LOGE("%s conn_id err %d\n", __func__, ret);
                goto __usage;
            }

            ret = dm_gattc_discover(conn_id);
        }
        else if (os_strcmp(argv[2], "write") == 0 && argc >= 6)
        {
            uint16_t conn_id = 0, attr_handle = 0;
            uint32_t len = 255;

            ret = sscanf(argv[3], "%hu", &conn_id);

            if (ret != 1)
            {
                CLI_LOGE("%s conn_id err %d\n", __func__, ret);
                goto __usage;
            }

            ret = sscanf(argv[4], "%hu", &attr_handle);

            if (ret != 1)
            {
                CLI_LOGE("%s attr_handle err %d\n", __func__, ret);
                goto __usage;
            }

            len = strnlen(argv[5], len);

            ret = dm_gattc_write(conn_id, attr_handle, (uint8_t *)argv[5], len);
        }
        else
        {
            goto __usage;
        }
    }
    else if (os_strcmp(argv[1], "passkey") == 0)
    {
        uint8_t accept = 0;
        uint32_t passkey = 0;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%d", &passkey);

            if (ret != 1)
            {
                goto __usage;
            }

            accept = 1;
        }

        ret = dm_gatt_passkey_reply(accept, passkey);
    }
    else if (os_strcmp(argv[1], "security_method") == 0)
    {
        uint8_t iocap = 0;
        uint8_t authen_req = 0;
        uint8_t key_distr = 0;

        if (argc >= 4)
        {
            ret = sscanf(argv[2], "%hhu", &iocap);

            if (ret != 1)
            {
                goto __usage;
            }

            ret = sscanf(argv[3], "%hhu", &authen_req);

            if (ret != 1)
            {
                goto __usage;
            }

            if (argc >= 5)
            {
                ret = sscanf(argv[4], "%hhu", &key_distr);

                if (ret != 1)
                {
                    goto __usage;
                }
            }
        }
        else
        {
            goto __usage;
        }

        ret = dm_gatt_set_security_method(iocap, authen_req, key_distr);
    }
    else if (os_strcmp(argv[1], "create_bond") == 0 && argc >= 3)
    {
        uint32_t mac[6] = {0};
        uint8_t mac_final[6] = {0};
        //sscanf bug: cant detect %hhx
        ret = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac + 5,
                     mac + 4,
                     mac + 3,
                     mac + 2,
                     mac + 1,
                     mac);

        if (ret != 6)
        {
            CLI_LOGE("%s addr err %d\n", __func__, ret);
            goto __usage;
        }

        for (int i = 0; i < sizeof(mac_final); ++i)
        {
            mac_final[i] = (uint8_t)mac[i];
        }

        ret = dm_ble_gap_create_bond(mac_final);
    }
    else if (os_strcmp(argv[1], "remove_bond") == 0 && argc >= 3)
    {
        uint32_t mac[6] = {0};
        uint8_t mac_final[6] = {0};
        //sscanf bug: cant detect %hhx
        ret = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac + 5,
                     mac + 4,
                     mac + 3,
                     mac + 2,
                     mac + 1,
                     mac);

        if (ret != 6)
        {
            CLI_LOGE("%s addr err %d\n", __func__, ret);
            goto __usage;
        }

        for (int i = 0; i < sizeof(mac_final); ++i)
        {
            mac_final[i] = (uint8_t)mac[i];
        }

        ret = dm_ble_gap_remove_bond(mac_final);
    }
    else if (os_strcmp(argv[1], "clean_bond") == 0)
    {
        ret = dm_ble_gap_clean_bond();
    }
    else if (os_strcmp(argv[1], "update_param") == 0 && argc >= 3)
    {
        uint32_t mac[6] = {0};
        uint8_t mac_final[6] = {0};
        uint16_t interval = 0x20;
        uint16_t tout = 500;

        //sscanf bug: cant detect %hhx
        ret = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac + 5,
                     mac + 4,
                     mac + 3,
                     mac + 2,
                     mac + 1,
                     mac);

        if (ret != 6)
        {
            CLI_LOGE("%s addr err %d\n", __func__, ret);
            goto __usage;
        }

        for (int i = 0; i < sizeof(mac_final); ++i)
        {
            mac_final[i] = (uint8_t)mac[i];
        }

        if (argc >= 4)
        {
            ret = sscanf(argv[3], "%hu", &interval);

            if (ret != 1)
            {
                goto __usage;
            }

            if (argc >= 5)
            {
                ret = sscanf(argv[3], "%hu", &tout);

                if (ret != 1)
                {
                    goto __usage;
                }
            }
        }

        ret = dm_ble_gap_update_param(mac_final, interval, tout);
    }
    else
    {
        goto __usage;
    }

    if (ret)
    {
        goto __error;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    ble_gatt_demo_usage();

__error:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_ble_gatt_commands[] =
{
    {"ble_gatt_demo", "see -h", cmd_ble_gatt_demo},
};

int cli_ble_gatt_demo_init(void)
{
    return cli_register_commands(s_ble_gatt_commands, sizeof(s_ble_gatt_commands) / sizeof(s_ble_gatt_commands[0]));
}

