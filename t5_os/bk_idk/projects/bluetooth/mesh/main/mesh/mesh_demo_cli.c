#include "cli.h"
#include "mesh_demo_cli.h"

static void mesh_usage(void)
{
    CLI_LOGI("Usage:\n"
             "ble_mesh provision init provisioner\n"
             "ble_mesh provision init provisionee\n"
             "ble_mesh provision send_count 1500\n"
#if CONFIG_BT_MESH_SHELL
             "ble_mesh shell mesh help"
             "ble_mesh shell bt help"
#endif
            );
    return;
}

static void cmd_mesh_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;

    if (argc == 1)
    {
        goto __usage;
    }
    else if (os_strcmp(argv[1], "-h") == 0)
    {
        mesh_usage();
        msg = CLI_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }
    if(!strcmp(argv[1], "led"))
    {
        ble_mesh_led_sample_shell(argc - 2, &argv[2]);
    }
    else if(!strcmp(argv[1], "tmall"))
    {
    	ble_mesh_tmall_spirit_sample_shell(argc - 2, &argv[2]);
    }
    else if(!strcmp(argv[1], "provision"))
    {
        bt_mesh_provision_sample_shell(argc - 2, &argv[2]);
    }
    else if(!strcmp(argv[1], "shell"))
    {
#if CONFIG_BT_MESH_SHELL
        if(!strcmp(argv[2], "mesh"))
        {
            zephry_mesh_shell_run(argc - 3, &argv[3]);
        }
        else if(!strcmp(argv[2], "bt"))
        {
            zephry_bt_shell_run(argc - 3, &argv[3]);
        }
        else
#endif
        {
            goto __usage;
        }
    }
    else
    {
        goto __usage;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__usage:
    mesh_usage();

    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static const struct cli_command s_mesh_commands[] =
{
    {"ble_mesh", "see -h", cmd_mesh_demo},
};


int cli_ble_mesh_demo_init(void)
{
    return cli_register_commands(s_mesh_commands, sizeof(s_mesh_commands) / sizeof(s_mesh_commands[0]));
}

