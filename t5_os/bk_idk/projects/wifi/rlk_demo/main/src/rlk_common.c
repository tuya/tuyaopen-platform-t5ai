#include "rlk_common.h"

int rlk_param_find_id(int argc, char **argv, char *param)
{
    int i;
    int index;

    index = RLK_INVALID_INDEX;
    if (NULL == param)
        goto find_over;

    for (i = 1; i < argc; i ++)
    {
        if (os_strcmp(argv[i], param) == 0) 
        {
            index = i;
            break;
        }
    }

find_over:
    return index;
}
int rlk_param_find(int argc, char **argv, char *param)
{
    int id;
    int find_flag = 0;

    id = rlk_param_find_id(argc, argv, param);
    if (RLK_INVALID_INDEX != id)
        find_flag = 1;

    return find_flag;
}

uint32_t rlk_sys_now(void)
{
	beken_time_t now_tick = 0;

	beken_time_get_time(&now_tick);
	return now_tick;
}

bk_err_t rlk_demo_dispatch(bk_rlk_recv_info_t *rx_info)
{
    bk_err_t ret = BK_OK;
    uint8_t *data = rx_info->data;
    RLK_APP_HDR *app_ptr = NULL;
    void* buf = NULL;
    
    app_ptr = (RLK_APP_HDR *)data;

    if (!rlk_demo_env.is_inited)
    {
        return BK_FAIL;
    }

    if((app_ptr->type == RLK_APP_TYPE_DATA)&&(app_ptr->subtype == RLK_APP_ECHO_RPLY))
    {
        buf = os_malloc(app_ptr->len);

        if(buf == NULL) return BK_FAIL;

        os_memcpy(buf,data,app_ptr->len);
        ret = rtos_push_to_queue(&rlk_ping_env.conn.recvmbox, &buf, BEKEN_NO_WAIT);
        
        if (ret != BK_OK)
        {
            os_free(buf);
            return BK_FAIL;
        }
    }
    else if((app_ptr->type == RLK_APP_TYPE_DATA)&&(app_ptr->subtype == RLK_APP_ECHO_REQ))
    {
        rlk_ping_response(rx_info);
    }

    return ret;

}
