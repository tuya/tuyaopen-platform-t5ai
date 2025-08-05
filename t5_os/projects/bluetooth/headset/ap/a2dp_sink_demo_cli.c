#include "cli.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#include "a2dp_sink/a2dp_sink_demo.h"
#include "hfp_hf/hfp_hf_demo.h"

static void headset_usage(void)
{
    CLI_LOGI("Usage:\n"
             "headset connect XX:XX:XX:XX:XX:XX\n"
             "headset disconnect XX:XX:XX:XX:XX:XX\n"
             "headset play\n"
             "headset pause\n"
             "headset next\n"
             "headset prev\n"
             "headset rewind XX\n"
             "headset fast_forward XX\n"
             "headset vol_up\n"
             "headset vol_down\n"
             "headset set_delay_value XX\n"
             "headset get_delay_value\n"
            );

    return;
}

static void cmd_headset_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
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
    else if (os_strcmp(argv[1], "connect") == 0)
    {
        if (argc >= 3)
        {
            uint8_t mac_final[6] = {0};
            uint32_t mac[6] = {0};

            //sscanf bug: cant detect uint8_t size point
            ret = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x", //argv[1], "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                         mac + 5,
                         mac + 4,
                         mac + 3,
                         mac + 2,
                         mac + 1,
                         mac);

            if (ret != 6)
            {
                CLI_LOGE("param err, need mac, %d %s\n", ret, argv[1]);
                return;
            }

            for (uint8_t i = 0; i < sizeof(mac_final) / sizeof(mac_final[0]); ++i)
            {
                mac_final[i] = mac[i];
            }


            CLI_LOGI("%s mac %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
                     mac_final[5],
                     mac_final[4],
                     mac_final[3],
                     mac_final[2],
                     mac_final[1],
                     mac_final[0]);


            ret = bk_bt_a2dp_sink_connect(mac_final);

            if (ret)
            {
                goto __error;
            }
        }
        else
        {
            goto __usage;
        }
    }
    else if (os_strcmp(argv[1], "disconnect") == 0)
    {
        if (argc >= 3)
        {
            uint8_t mac_final[6] = {0};
            uint32_t mac[6] = {0};

            //sscanf bug: cant detect uint8_t size point
            ret = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x", //argv[1], "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                         mac + 5,
                         mac + 4,
                         mac + 3,
                         mac + 2,
                         mac + 1,
                         mac);

            if (ret != 6)
            {
                CLI_LOGE("param err, need mac, %d %s\n", ret, argv[1]);
                return;
            }

            for (uint8_t i = 0; i < sizeof(mac_final) / sizeof(mac_final[0]); ++i)
            {
                mac_final[i] = mac[i];
            }

            CLI_LOGI("%s mac %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
                     mac_final[5],
                     mac_final[4],
                     mac_final[3],
                     mac_final[2],
                     mac_final[1],
                     mac_final[0]);

            ret = bk_bt_a2dp_sink_disconnect(mac_final);

            if (ret)
            {
                goto __error;
            }
        }
        else
        {
            goto __usage;
        }
    }
    else if (os_strcmp(argv[1], "play") == 0)
    {
        void bk_bt_app_avrcp_ct_play(void);
        bk_bt_app_avrcp_ct_play();
    }
    else if (os_strcmp(argv[1], "pause") == 0)
    {
        void bk_bt_app_avrcp_ct_pause(void);
        bk_bt_app_avrcp_ct_pause();
    }
    else if (os_strcmp(argv[1], "prev") == 0)
    {
        void bk_bt_app_avrcp_ct_prev(void);
        bk_bt_app_avrcp_ct_prev();
    }
    else if (os_strcmp(argv[1], "next") == 0)
    {
        void bk_bt_app_avrcp_ct_next(void);
        bk_bt_app_avrcp_ct_next();
    }
    else if (os_strcmp(argv[1], "rewind") == 0)
    {
        int32_t option = 500;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%d", &option);

            if (ret != 1)
            {
                CLI_LOGE("param err\n");
                goto __usage;
            }
        }

        void bk_bt_app_avrcp_ct_rewind(uint32_t ms);
        bk_bt_app_avrcp_ct_rewind(option);
    }
    else if (os_strcmp(argv[1], "fast_forward") == 0)
    {
        int32_t option = 500;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%d", &option);

            if (ret != 1)
            {
                CLI_LOGE("param err\n");
                goto __usage;
            }
        }

        void bk_bt_app_avrcp_ct_fast_forward(uint32_t ms);
        bk_bt_app_avrcp_ct_fast_forward(option);
    }
    else if (os_strcmp(argv[1], "vol_up") == 0)
    {
        void bk_bt_app_avrcp_ct_vol_up(void);
        bk_bt_app_avrcp_ct_vol_up();
    }
    else if (os_strcmp(argv[1], "vol_down") == 0)
    {
        void bk_bt_app_avrcp_ct_vol_down(void);
        bk_bt_app_avrcp_ct_vol_down();
    }
    else if (os_strcmp(argv[1], "pair_mode") == 0)
    {
        void bk_bt_enter_pairing_mode(void);
        bk_bt_enter_pairing_mode();
    }
    else if (os_strcmp(argv[1], "set_delay_value") == 0)
    {
        if (argc >= 3)
        {
            uint16_t delay_value = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
            ret = bk_bt_a2dp_sink_set_delay_value(delay_value);

            if (ret)
            {
                CLI_LOGE("%s a2dp sink set delay value err %d\n", __func__, ret);
                goto __error;
            }
        }
        else
        {
            goto __usage;
        }
    }
    else if (os_strcmp(argv[1], "get_delay_value") == 0)
    {
        ret = bk_bt_a2dp_sink_get_delay_value();

        if (ret)
        {
            CLI_LOGE("%s a2dp sink set delay value err %d\n", __func__, ret);
            goto __error;
        }
    }
    else if(os_strcmp(argv[1], "get_attr") == 0)
    {
        int32_t attr = 0;

        if (argc >= 3)
        {
            ret = sscanf(argv[2], "%d", &attr);

            if (ret != 1)
            {
                goto __error;
            }
        }

        bk_bt_app_avrcp_ct_get_attr(attr);
    }
    else if(os_strcmp(argv[1], "vr") == 0 && argc >= 3)
    {
        uint8_t enable = 0;

        ret = sscanf(argv[2], "%hhu", &enable);

        if (ret != 1)
        {
            goto __error;
        }

        hfp_demo_vr(enable);
    }
    else if(os_strcmp(argv[1], "dial") == 0 && argc >= 3)
    {
        uint8_t enable = 0;
        uint8_t number[32] = "112";

        ret = sscanf(argv[2], "%hhu", &enable);

        if (ret != 1)
        {
            goto __error;
        }

        if (argc >= 4)
        {
            ret = sscanf(argv[3], "%31s", number);

            if (ret != 1)
            {
                goto __error;
            }
        }

        hfp_demo_dial(enable, number);
    }
    else if(os_strcmp(argv[1], "answer") == 0 && argc >= 3)
    {
        uint8_t accept = 0;
        uint8_t number[32] = "112";

        ret = sscanf(argv[2], "%hhu", &accept);

        if (ret != 1)
        {
            goto __error;
        }

        hfp_demo_answer(accept);
    }
    else if(os_strcmp(argv[1], "hfpcmd") == 0 && argc >= 3)
    {
        uint8_t accept = 0;
        uint8_t cmd[64] = {0};

        ret = sscanf(argv[2], "%63s", cmd);

        if (ret != 1)
        {
            goto __error;
        }

        hfp_demo_cust_cmd(cmd);
    }
    else
    {
        goto __usage;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    headset_usage();

__error:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_headset_commands[] =
{
    {"headset", "see -h", cmd_headset_demo},
};

int cli_headset_demo_init(void)
{
    return cli_register_commands(s_headset_commands, sizeof(s_headset_commands) / sizeof(s_headset_commands[0]));
}

