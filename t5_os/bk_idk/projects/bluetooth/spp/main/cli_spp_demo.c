#include "bk_cli.h"
#include <common/sys_config.h>
#include <common/bk_kernel_err.h>
#include <string.h>
#include <stdlib.h>
#include "cli.h"
#include "components/bluetooth/bk_dm_spp.h"
#include "components/bluetooth/bk_dm_bt.h"

#define SPP_CMD_CNT (sizeof(s_spp_commands) / sizeof(struct cli_command))


#define CMD_RSP_SUCCEED               "CLI BT RSP:OK\r\n"
#define CMD_RSP_ERROR                 "CLI BT RSP:ERROR\r\n"

extern int bk_rand(void);

#define DO1(buf) crc = crc_table(((int)crc ^ (*buf++)) & 0xff) ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
static uint32_t crc_table(uint32_t index)
{
	uint32_t c = index;
	uint32_t poly = 0xedb88320L;
	int k;

	for (k = 0; k < 8; k++)
		c = c & 1 ? poly ^ (c >> 1) : c >> 1;

	return c;
}

uint32_t crc32(uint32_t crc, const uint8_t *buf, int len)
{
    if (buf == NULL) return 0L;

  //  crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
      DO8(buf);
      len -= 8;
    }
    if (len) do {
      DO1(buf);
    } while (--len);
    return crc;// ^ 0xffffffffL;
}


static unsigned int crc32_table[256];

unsigned int make_crc32_table(void)
{
	uint32_t c;
	int i = 0;
	int bit = 0;

	for(i = 0;i < 256;i++)
	{
		c = (unsigned int)i;

		for(bit = 0;bit < 8;bit++)
		{
			if(c&1)
			{
				c = (c>>1)^(0xEDB88320);
			}
			else
			{
				c = c >> 1;
			}
		}

		crc32_table[i] = c;
//		os_printf("crc32_table[%d] = %08x\r\n",i,crc32_table[i]);
	}

	return 0;
}

uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, int len)
{
	while(len--)
	{
		crc = (crc >> 8)^(crc32_table[(crc^*buf++)&0xff]);
	}

	return crc;
}


static int get_addr_from_param(bk_bd_addr_t bdaddr, char *input_param)
{
    int err = kNoErr;
    uint8_t addr_len = os_strlen(input_param);
    char temp [ 3 ];
    uint8_t j = 1;
    uint8_t k = 0;

    if (addr_len != (BK_BD_ADDR_LEN * 2 + 5))
    {
        err = kParamErr;
        return err;
    }

    for (uint8_t i = 0; i < addr_len; i++)
    {
        if (input_param [ i ] >= '0' && input_param [ i ] <= '9')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] >= 'a' && input_param [ i ] <= 'f')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] >= 'A' && input_param [ i ] <= 'F')
        {
            temp [ k ] = input_param [ i ];
            k += 1;
        }
        else if (input_param [ i ] == ':')
        {
            temp [ k ] = '\0';
            bdaddr [ BK_BD_ADDR_LEN - j ] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
            j++;
        }
        else
        {
            err = kParamErr;
            return err;
        }

        if (i == (addr_len - 1))
        {
            temp [ k ] = '\0';
            bdaddr [ BK_BD_ADDR_LEN - j ] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
        }
    }

    return kNoErr;
}

#define CLI_SPP_MAX_DEVS        10
#define CLI_SPP_W4_DISCOVER     1<<1
#define CLI_SPP_W4_CONN         1<<2
#define CLI_SPP_W4_DISCONN      1<<3
#define CLI_SPP_W4_SRV_START    1<<4
#define CLI_SPP_W4_SRV_OPEN     1<<5
#define CLI_SPP_W4_SRV_STOP     1<<6
#define CLI_SPP_W4_DEINIT       1<<7
#define CLI_SPP_W4_TX           1<<8
#define CLI_SPP_W4_RATE_START   1<<9
#define CLI_SPP_W4_RATE_TX      1<<10
#define CLI_SPP_W4_RATE_RX      1<<11
#define CLI_SPP_W4_RATE_END     1<<12

#define CLI_SPP_TX_BUFF_SIZE  1024

static const char *tx_through_cmd_start = "spp tx throught start";
static const char *tx_through_cmd_end = "spp tx throught end";

typedef struct{
    bk_bd_addr_t remote_addr;
    uint8_t remote_chl;
    uint8_t local_chl;
    uint32_t handle;
    uint8_t used;
    uint32_t pending;
    uint8_t conn;
    uint8_t id;
    //tx rate test
    uint32_t rate_tx_total_len;
    uint32_t rate_tx_len;
    uint32_t rate_tx_time;
    uint32_t rate_crc;
    uint8_t *rate_buf;
}cli_spp_dev_t;

static cli_spp_dev_t spp_devs[CLI_SPP_MAX_DEVS] = {0};

static int get_free_spp_dev(cli_spp_dev_t **dev)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(!spp_devs[i].used)
        {
            spp_devs[i].id = i;
            spp_devs[i].used = 1;
            *dev = &spp_devs[i];
            return i+1;
        }
    }
    return 0;
}

static int get_spp_dev_by_addr(cli_spp_dev_t **dev, bk_bd_addr_t addr)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used)
        {
            if(!os_memcmp(addr, spp_devs[i].remote_addr, 6))
            {
                *dev = &spp_devs[i];
                return i+1;
            }
        }
    }
    return 0;
}

static int get_spp_dev_by_addr_handle(cli_spp_dev_t **dev, bk_bd_addr_t addr, uint8_t handle)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used)
        {
            if(!os_memcmp(addr, spp_devs[i].remote_addr, 6) && spp_devs[i].handle == handle)
            {
                *dev = &spp_devs[i];
                return i+1;
            }
        }
    }
    return 0;
}


static int get_spp_dev_by_handle(cli_spp_dev_t **dev, uint32_t handle)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used)
        {
            if(spp_devs[i].handle == handle && spp_devs[i].conn)
            {
                *dev = &spp_devs[i];
                return i+1;
            }
        }
    }
    return 0;
}

static int get_spp_srv_dev_by_handle(cli_spp_dev_t **dev, uint32_t handle)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used)
        {
            if(spp_devs[i].handle == handle)
            {
                *dev = &spp_devs[i];
                return i+1;
            }
        }
    }
    return 0;
}


static int get_blocking_spp_dev(cli_spp_dev_t **dev, uint32_t w4_mask)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used && (spp_devs[i].pending&(w4_mask)))
        {
            *dev = &spp_devs[i];
            return i+1;
        }
    }
    return 0;
}

static int spp_busy()
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used && (spp_devs[i].pending))
        {
            return i+1;
        }
    }
    return 0;
}

static int get_spp_dev_by_chnl(cli_spp_dev_t **dev, uint8_t chnl)
{
    for(int i=0;i<CLI_SPP_MAX_DEVS;i++)
    {
        if(spp_devs[i].used && (spp_devs[i].local_chl == chnl))
        {
            *dev = &spp_devs[i];
            return i+1;
        }
    }
    return 0;
}

static void free_spp_dev(int i)
{
    if(i < CLI_SPP_MAX_DEVS && spp_devs[i].used)
    {
        memset(&spp_devs[i], 0, sizeof(cli_spp_dev_t));
    }
}

static int bt_spp_rate_poll(cli_spp_dev_t *dev, uint16_t len)
{
    if(!dev->pending)
    {
        uint8_t *tx_data = (uint8_t *)os_malloc(CLI_SPP_TX_BUFF_SIZE);
        if(tx_data ==NULL)
        {
            CLI_LOGE("%s, malloc fail \r\n", __func__);
            return -1;
        }
        dev->rate_buf = tx_data;
        dev->pending |= CLI_SPP_W4_TX|CLI_SPP_W4_RATE_START;
        uint8_t data[50] = {0};
        os_memcpy(data, tx_through_cmd_start, os_strlen(tx_through_cmd_start));
        bk_spp_write(dev->handle, os_strlen(tx_through_cmd_start),data);
        return 0;
    }else if(dev->pending&CLI_SPP_W4_RATE_START)
    {
        dev->pending &=~CLI_SPP_W4_RATE_START;
        dev->rate_crc = 0xFFFFFFFF;
        make_crc32_table();
        dev->rate_tx_time = rtos_get_time();
        dev->pending|=CLI_SPP_W4_RATE_TX;
        CLI_LOGI("========spp tx start total_length: %d ========\r\n", dev->rate_tx_total_len);
    }else if(dev->pending&CLI_SPP_W4_RATE_TX)
    {
        dev->rate_tx_len += len;
        uint64_t current_time = rtos_get_time();
        float spend_time = (float)(current_time - dev->rate_tx_time)/1000;
        CLI_LOGI("---->spend time: %f s\r\n", spend_time);
        float speed = (float)dev->rate_tx_len/1024/spend_time;
        dev->rate_crc = calc_crc32(dev->rate_crc, dev->rate_buf, len);
        CLI_LOGI("spp tx length: %d, speed: %.3fKB/s \r\n", dev->rate_tx_len, speed);
    }else if(dev->pending&CLI_SPP_W4_RATE_END)
    {
        CLI_LOGI("========spp tx finish tx_length: %d, crc:0x%x ========\r\n", dev->rate_tx_len, dev->rate_crc);
        if(dev->rate_buf)
        {
            os_free(dev->rate_buf);
        }
        dev->pending &= ~CLI_SPP_W4_RATE_END;
        return 0;
    }
    if(dev->rate_tx_len < dev->rate_tx_total_len)
    {
        uint32_t remain_len = dev->rate_tx_total_len - dev->rate_tx_len;
        uint32_t tx_len = (remain_len>=CLI_SPP_TX_BUFF_SIZE) ? CLI_SPP_TX_BUFF_SIZE:remain_len;
        uint8_t rand = bk_rand() & 0xff;
        os_memset(dev->rate_buf, rand, tx_len);
        bk_spp_write(dev->handle, tx_len, dev->rate_buf);
    }else
    {
        uint8_t data[50] = {0};
        os_memcpy(data, tx_through_cmd_end, os_strlen(tx_through_cmd_end));
        bk_spp_write(dev->handle, os_strlen(tx_through_cmd_end), data);
        dev->pending &= ~CLI_SPP_W4_RATE_TX;
        dev->pending |= CLI_SPP_W4_RATE_END;
    }
    return 0;
}

static void cli_spp_help(void)
{
    CLI_LOGI("spp init\n");
    CLI_LOGI("spp deinit\n");
    CLI_LOGI("spp conn [remote_addr]\n");
    CLI_LOGI("spp disconn [remote_addr] [spp_handle]\n");
    CLI_LOGI("spp start_server\n");
    CLI_LOGI("spp stop_server [local_channel]\n");
    CLI_LOGI("spp write [handle][data]\n");
    CLI_LOGI("spp rate [length]\n");
    CLI_LOGI("spp conn1 [remote_addr] [remote_channel]\r\n");
}

void bk_cli_bt_spp_callback(bk_spp_cb_event_t event, bk_spp_cb_param_t *param)
{
    switch (event)
    {
        case BK_SPP_INIT_EVT:
            CLI_LOGI("%s spp init status:%d\r\n", __func__, param->init.status);
            break;
        case BK_SPP_UNINIT_EVT:
            CLI_LOGI("%s spp deinit status:%d\r\n", __func__, param->uninit.status);
            break;
        case BK_SPP_DISCOVERY_COMP_EVT:
        {
            cli_spp_dev_t *dev;
            if(get_blocking_spp_dev(&dev, CLI_SPP_W4_DISCOVER|CLI_SPP_W4_CONN))
            {
                if(param->disc_comp.status == BK_SPP_SUCCESS)
                {
                    CLI_LOGI("%s, spp discover success, chnl0:%d, cnt:%d !!\r\n", __func__, param->disc_comp.scn[0], param->disc_comp.scn_num);
                    dev->remote_chl = param->disc_comp.scn[0];
                    bk_spp_connect(BK_SPP_ROLE_MASTER, param->disc_comp.scn[0], dev->remote_addr);
                    dev->pending&=(~CLI_SPP_W4_DISCOVER);
                }else
                {
                    CLI_LOGI("%s, spp discover fail !!\r\n", __func__);
                    dev->pending = 0;
                }
            }else
            {
                CLI_LOGI("%s, spp discover finish, no w4_dev find!!\r\n", __func__);
            }
        }
        break;
        case BK_SPP_OPEN_EVT:
        {
            cli_spp_dev_t *dev;
            if(param->open.status == BK_SPP_SUCCESS)
            {
                if(get_blocking_spp_dev(&dev, CLI_SPP_W4_CONN))
                {
                    dev->handle = param->open.handle;
                    dev->conn = 1;
                    dev->pending &= ~CLI_SPP_W4_CONN;
                }
                CLI_LOGI("%s, spp conn success to 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\r\n", __func__, 
                        param->open.rem_bda[0],param->open.rem_bda[1],param->open.rem_bda[2],
                        param->open.rem_bda[3],param->open.rem_bda[4],param->open.rem_bda[5]);
                CLI_LOGI("HANDLE: 0x%02x \r\n", dev->handle);
            }else
            {
                if(get_blocking_spp_dev(&dev, CLI_SPP_W4_CONN))
                {
                    dev->pending = 0;
                }
                CLI_LOGI("%s, spp conn fail \r\n", __func__);
            }
        }
        break;
        case BK_SPP_CLOSE_EVT:
        {
            cli_spp_dev_t *dev;
            if(get_blocking_spp_dev(&dev, CLI_SPP_W4_DISCONN))
            {
                if(param->close.status == BK_SPP_SUCCESS)
                {
                    dev->conn = 0;
                    dev->pending&=~CLI_SPP_W4_DISCONN;
                    CLI_LOGI("%s, spp disconn success \r\n", __func__);
                }else
                {
                    CLI_LOGI("%s, spp disconn fail \r\n", __func__);
                }
            }else if(get_spp_srv_dev_by_handle(&dev, param->close.handle))
            {
                if(param->close.status == BK_SPP_SUCCESS)
                {
                    dev->conn = 0;
                    CLI_LOGI("%s, spp srv disconn success \r\n", __func__);
                }else
                {
                    CLI_LOGI("%s, spp srv disconn fail \r\n", __func__);
                }
            }
            else
            {
                CLI_LOGI("%s, spp disconn, no dev found \r\n", __func__);
            }
        }
            break;
        case BK_SPP_START_EVT:
        {
            cli_spp_dev_t *dev;
            if(get_blocking_spp_dev(&dev, CLI_SPP_W4_SRV_START))
            {
                if(param->start.status == BK_SPP_SUCCESS)
                {
                    dev->local_chl = param->start.scn;
                    dev->handle = param->start.handle;
                    CLI_LOGI("%s, spp start_server success, chnl:%d, spp_handle:0x%02x \r\n", __func__, param->start.scn, dev->handle);
                }else
                {
                    CLI_LOGI("%s, spp start_server fail \r\n", __func__);
                    free_spp_dev(dev->id);
                }
                dev->pending &= ~CLI_SPP_W4_SRV_START;
            }else
            {
                CLI_LOGI("%s, spp start_server, no dev found \r\n", __func__);
            }
        }
        break;
        case BK_SPP_CL_INIT_EVT:
            break;
        case BK_SPP_DATA_IND_EVT:
        {
            static float speed;
            cli_spp_dev_t *dev;
            if(get_spp_dev_by_handle(&dev, param->data_ind.handle))
            {
                if(!os_strncmp((char *)param->data_ind.data, tx_through_cmd_start, param->data_ind.len))
                {
                    CLI_LOGI("======== spp rx start ========\r\n");
                    dev->pending |= CLI_SPP_W4_RATE_RX;
                    dev->rate_crc = 0xFFFFFFFF;
                    dev->rate_tx_len = 0;
                    make_crc32_table();
                    dev->rate_tx_time = rtos_get_time();
                }else if(!os_strncmp((char *)param->data_ind.data, tx_through_cmd_end, param->data_ind.len))
                {
                    CLI_LOGI("========spp rx finish tx_length: %d, speed: %.3fKB/s, crc:0x%x========\r\n", dev->rate_tx_len, speed, dev->rate_crc);
                    dev->pending &= ~CLI_SPP_W4_RATE_RX;
                }else
                {
                    if(dev->pending&CLI_SPP_W4_RATE_RX)
                    {
                        dev->rate_tx_len += param->data_ind.len;
                        uint64_t current_time = rtos_get_time();
                        float spend_time = (float)(current_time - dev->rate_tx_time)/1000;
                        CLI_LOGI("---->spend time: %f s\r\n", spend_time);
                        speed = (float)dev->rate_tx_len/1024/spend_time;
                        dev->rate_crc = calc_crc32(dev->rate_crc, param->data_ind.data, param->data_ind.len);
                        CLI_LOGI("spp rx length: %d, speed: %.3fKB/s \r\n", dev->rate_tx_len, speed);
                    }else
                    {
                        CLI_LOGI("===========DATA IND===========\r\n");
                        CLI_LOGI("%s, spp data ind, handle:0x%02x, len:%d \r\n", __func__, param->data_ind.handle, param->data_ind.len);
                        if(param->data_ind.len < 100)
                        {
                            char data[100] = {0};
                            os_memcpy(data, param->data_ind.data, param->data_ind.len);
                            CLI_LOGI("%s \r\n", data);
                        }
                        CLI_LOGI("==============================\r\n");
                    }
                }
            }else
            {
                CLI_LOGI("%s, spp data ind, no handle:0x%02x dev found \r\n", __func__, param->data_ind.handle);
            }
        }
        break;
        case BK_SPP_WRITE_EVT:
        {
            cli_spp_dev_t *dev;
            if(get_spp_dev_by_handle(&dev, param->write.handle))
            {
                dev->pending &= ~CLI_SPP_W4_TX;
                if(dev->pending)
                {
                    bt_spp_rate_poll(dev, param->write.len);
                }else
                {
                    CLI_LOGI("%s, spp write cnf, handle 0x%02x\r\n", __func__, dev->handle);
                }
            }else
            {
                CLI_LOGI("%s, spp write cnf, no dev found \r\n", __func__);
            }
        }
        break;
        case BK_SPP_SRV_OPEN_EVT:
        {
            cli_spp_dev_t *dev;
            if(param->srv_open.status == BK_SPP_SUCCESS)
            {
                if(get_spp_srv_dev_by_handle(&dev, param->srv_open.handle))
                {
                    dev->conn = 1;
                    dev->handle = param->srv_open.handle;
                    CLI_LOGI("%s, spp conn success to 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\r\n", __func__, 
                        param->open.rem_bda[0],param->open.rem_bda[1],param->open.rem_bda[2],
                        param->open.rem_bda[3],param->open.rem_bda[4],param->open.rem_bda[5]);
                    CLI_LOGI("HANDLE: 0x%02x \r\n", dev->handle);
                }else
                {
                    CLI_LOGI("%s, spp server conn ind, no dev found \r\n", __func__);
                }
            }
        }
        break;
        case BK_SPP_SRV_STOP_EVT:
        {
            cli_spp_dev_t *dev;
            if(get_blocking_spp_dev(&dev, CLI_SPP_W4_SRV_STOP) || get_spp_dev_by_chnl(&dev, param->srv_stop.scn))
            {
                if(param->srv_stop.status == BK_SPP_SUCCESS)
                {
                    dev->pending &= ~CLI_SPP_W4_SRV_STOP;
                    CLI_LOGI("%s, spp stop_server success, handle: 0x%x, chnl:%d \r\n", __func__, dev->handle, param->srv_stop.scn);
                    free_spp_dev(dev->id);
                }else
                {
                    CLI_LOGI("%s, spp stop_server fail \r\n", __func__);
                }
            }else
            {
                CLI_LOGI("%s, spp stop_server, no dev found \r\n", __func__);
            }
        }
        break;
        default :
            break;
    }
}

void spp_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *cmd_rsp = CMD_RSP_SUCCEED;
    bk_spp_register_callback(bk_cli_bt_spp_callback);
    if(spp_busy())
    {
        CLI_LOGE("spp blocking now!!! \r\n");
        goto error;
    }
    if (strcmp(argv [ 1 ], "help") == 0)
    {
        cli_spp_help();
    }
    else if (!strcmp(argv [ 1 ], "init"))
    {
        bk_spp_init();
    }
    else if (!strcmp(argv [ 1 ], "deinit"))
    {
        bk_spp_deinit();
    }
    else if (!strcmp(argv [ 1 ], "conn"))
    {
        bk_bd_addr_t addr;
        if(get_addr_from_param(addr, argv [ 2 ]))
        {
            CLI_LOGE("spp conn addr error \r\n");
            goto error;
        }
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_addr(&dev, addr))
        {
        }
        else if(get_free_spp_dev(&dev))
        {
            memcpy(dev->remote_addr, addr, 6);
        }else
        {
            CLI_LOGI("spp max dev \r\n");
            goto error;
        }
        dev->pending |= CLI_SPP_W4_DISCOVER;
        dev->pending |= CLI_SPP_W4_CONN;
        bk_spp_start_discovery(addr);
    }
    else if(!strcmp(argv [ 1 ], "disconn"))
    {
        bk_bd_addr_t addr;
        if(get_addr_from_param(addr, argv [ 2 ]))
        {
            CLI_LOGE("spp disconn addr error \r\n");
            goto error;
        }
        uint8_t handle = os_strtoul(argv[3], NULL, 16);
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_addr_handle(&dev, addr, handle))
        {
            if(dev->conn)
            {
                dev->pending |= CLI_SPP_W4_DISCONN;
                bk_spp_disconnect(dev->handle);
            }else
            {
                CLI_LOGI("spp disconn, dev not conn\r\n");
            }
        }else
        {
            CLI_LOGI("spp disconn, not find dev \r\n");
        }
    }
    else if(!strcmp(argv [ 1 ], "start_server"))
    {
        cli_spp_dev_t *dev;
        if(get_free_spp_dev(&dev))
        {
            dev->local_chl = 0;
            dev->pending |= CLI_SPP_W4_SRV_START;
            bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
            bk_spp_start_srv(BK_SPP_ROLE_SLAVE, 0, NULL);
        }else
        {
            CLI_LOGI("spp max dev \r\n");
            goto error;
        }
    }
    else if(!strcmp(argv [ 1 ], "stop_server"))
    {
        uint8_t chnl = os_strtoul(argv[2], NULL, 10);
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_chnl(&dev,  chnl))
        {
            dev->pending |= CLI_SPP_W4_SRV_STOP;
            bk_spp_stop_srv_scn(chnl);
        }else if(chnl == 0)
        {
            bk_spp_stop_srv();
        }
        {
            CLI_LOGI("spp stop srv, not find chnnl:%d dev\r\n", chnl);
        }
    }
    else if(!strcmp(argv [ 1 ], "write"))
    {
        uint32_t handle = os_strtoul(argv[2], NULL, 16);
        char *data = argv[3];
        uint16_t data_len = os_strlen(data);
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_handle(&dev, handle))
        {
            dev->pending |= CLI_SPP_W4_TX;
            bk_spp_write(handle, data_len, (uint8_t *)data);
        }else
        {
            CLI_LOGI("spp write, not find handle:0x%02x dev\r\n", handle);
            goto error;
        }
    }
    else if(!strcmp(argv [ 1 ], "rate"))
    {
        uint32_t handle = os_strtoul(argv[2], NULL, 16);
        uint32_t tx_len = os_strtoul(argv[3], NULL, 16);
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_handle(&dev, handle))
        {
            dev->rate_tx_total_len = tx_len;
            dev->rate_tx_len = 0;
            bt_spp_rate_poll(dev, 0);
        }else
        {
            CLI_LOGI("spp rate test, not find handle:0x%02x dev\r\n", handle);
            goto error;
        }
    }
    else if(!strcmp(argv [ 1 ], "status"))
    {
        uint32_t handle = os_strtoul(argv[2], NULL, 16);
        cli_spp_dev_t *dev;
        if(get_spp_dev_by_handle(&dev, handle))
        {
            CLI_LOGI("pending:0x%x \r\n", dev->pending);
        }else
        {
            CLI_LOGI("spp status get, not find handle:0x%02x dev\r\n", handle);
            goto error;
        }
    }
    else if(!strcmp(argv [ 1 ], "conn1"))
    {
        bk_bd_addr_t addr;
        if(get_addr_from_param(addr, argv [ 2 ]))
        {
            CLI_LOGE("spp conn addr error \r\n");
            goto error;
        }
        uint8_t chl = os_strtoul(argv[3], NULL, 10);
        cli_spp_dev_t *dev;
        if(get_free_spp_dev(&dev))
        {
            memcpy(dev->remote_addr, addr, 6);
        }else
        {
            CLI_LOGI("spp max dev \r\n");
            goto error;
        }
        dev->remote_chl = chl;
        dev->pending |= CLI_SPP_W4_CONN;
        bk_spp_connect(BK_SPP_ROLE_MASTER, dev->remote_chl, dev->remote_addr);
    }
    else
    {
        goto error;
    }
    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
    return;
error:
    cmd_rsp = CMD_RSP_ERROR;
    memcpy(pcWriteBuffer, cmd_rsp, strlen(cmd_rsp));
}

static const struct cli_command s_spp_commands[] =
{
#if CONFIG_BLUETOOTH
    {"spp", "spp arg1 arg2 ... argn",  spp_command},
#endif

};

int spp_demo_cli_init(void)
{
    return cli_register_commands(s_spp_commands, SPP_CMD_CNT);
}
