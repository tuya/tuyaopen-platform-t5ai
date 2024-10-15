#include "rlk_common.h"


#define RLK_CMD_CNT (sizeof(s_rlk_commands) / sizeof(struct cli_command))


void cli_rlk_ping_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t id;
    //sscanf bug: cant detect uint8_t size point
    uint32_t mac_addr_temp[6];
    if (rlk_param_find(argc, argv, "-h") || (argc == 1))
        goto __usage;
    else if (rlk_param_find(argc, argv, "-stop") || rlk_param_find(argc, argv, "--stop"))
    {
        rlk_ping_stop();
        return;
    }
    memcpy(rlk_ping_env.mac_str,argv[1],RLK_MAC_STRING_LEN);
    //input 3 section
    if(strlen(argv[1]) < 9)
    {
        rlk_ping_env.mac_addr[0] = 0xc8;
        rlk_ping_env.mac_addr[1] = 0x47;
        rlk_ping_env.mac_addr[2] = 0x8c;
        sscanf(argv[1], "%x:%x:%x",&mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);
        
        rlk_ping_env.mac_addr[3] = mac_addr_temp[3];
        rlk_ping_env.mac_addr[4] = mac_addr_temp[4];
        rlk_ping_env.mac_addr[5] = mac_addr_temp[5];
    }//input 6 section
    else
    {
        sscanf(argv[1], "%x:%x:%x:%x:%x:%x",
        &mac_addr_temp[0], &mac_addr_temp[1], &mac_addr_temp[2],
        &mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);

        for(int i=0;i<6;i++)
        {
            rlk_ping_env.mac_addr[i] = (uint8_t)(mac_addr_temp[i] & 0xff);
        }
    }
    id = rlk_param_find_id(argc, argv, "-c");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.count = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-i");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.interval = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-s");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.size = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-t");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.timeout = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    rlk_ping_default_set();

    RLK_LOGI("c=%d,s=%d,i=%d,t=%d\n",rlk_ping_env.count,rlk_ping_env.size,rlk_ping_env.interval,rlk_ping_env.timeout);
    RLK_LOGI("rlk_ping_env.mac_addr = ");
    for(int i =0; i<6;i++)
    {
        RLK_LOGI("0x%x ",rlk_ping_env.mac_addr[i]);
    }
    RLK_LOGI("\n");
    RLK_LOGI("rlk_ping_env.mac_addr_str = %s\n",rlk_ping_env.mac_str);

    rlk_ping_start();

    return;

__usage:
    rlk_ping_usage();
}


static const struct cli_command s_rlk_commands[] = {
    {"rlk_ping", "rlk_ping <mac>", cli_rlk_ping_cmd},
};

int cli_rlk_init(void)
{
    return cli_register_commands(s_rlk_commands, RLK_CMD_CNT);
}

