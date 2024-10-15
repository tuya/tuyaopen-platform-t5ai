#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"

#if CONFIG_OTA_HTTP
#include "utils_httpc.h"
#include "modules/wifi.h"
#endif

#ifdef CONFIG_HTTP_AB_PARTITION
#define OTA_DEBUG_TEST                    (0)
#define FLASH_BASE_ADDRESS                (0x44030000)
#define FLASH_OFFSET_ENABLE               (0x19)
#define OTA_FINA_EXEC_FLAG_OFFSET         (0x0)
#define OTA_TEMP_EXEC_FLAG_OFFSET         (0x4)
#define OTA_CUSTM_CONF_FLAG_OFFSET        (0x8)
#define OTA_DOWNLOAD_STATUS_FLAG_OFFSET   (0xC) //0xFE.download start. //0xFC download success

part_flag update_part_flag;

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
static uint8 ota_get_flash_offset_enable_value(void)
{
	uint8 ret_val;

	ret_val = (REG_READ((FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4)) & 0x1);
	os_printf("ret_val  :0x%x\r\n",ret_val);

	return ret_val;
}

void bk_ota_double_check_for_execution(void)
{
	uint8 ret;
	
	ret = bk_ota_get_current_partition();
	if(ret == 0)
	{
		os_printf("confirm exec pos_ind part a \r\n");
		bk_ota_confirm_update_partition(CONFIRM_EXEC_A);
	}
	else
	{
		os_printf("confirm exec pos_ind part b \r\n");
		bk_ota_confirm_update_partition(CONFIRM_EXEC_B);
	}
}
#endif // (CONFIG_TFM_FWU)

uint8 bk_ota_get_current_partition(void)
{
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
	uint8 ret_val;
	
	ret_val = ota_get_flash_offset_enable_value();

	return ret_val;  //ret_val: 0x0 represents A 0x1 :represents B.
#else
	exec_flag ota_exec_flag = 5;
	bk_logic_partition_t *bk_ptr = NULL;

	bk_ptr = bk_flash_partition_get_info(BK_PARTITION_OTA_FINA_EXECUTIVE); 
	os_printf("bk_ptr->partition_start_addr  :0x%x\r\n",bk_ptr->partition_start_addr);
	bk_flash_read_bytes(bk_ptr->partition_start_addr ,(uint8_t *)&ota_exec_flag, sizeof(u8));

	return ota_exec_flag ;  // ota_exec_flag :0x0/0xFF represents A 0x1 :represents B 

#endif
}

void ota_write_flash(bk_partition_t ota_partition_flag, u8 flag, u8 offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 ota_final_buff[1],ota_temp_buff[1],ota_cconfirm_buff[1],ota_download_status_buff[1];
#if OTA_DEBUG_TEST
	u8 temp1_buff[1],temp2_buff[1],temp3_buff[1],temp4_buff[1];
#endif
	flash_protect_type_t protect_type;

	bk_ptr = bk_flash_partition_get_info(ota_partition_flag);
    
	os_printf("ota_write_flash:partition_start_addr:0x%x\r\n",(bk_ptr->partition_start_addr));
	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_final_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_TEMP_EXEC_FLAG_OFFSET),(uint8_t *)ota_temp_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_CUSTM_CONF_FLAG_OFFSET),(uint8_t *)ota_cconfirm_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_DOWNLOAD_STATUS_FLAG_OFFSET),(uint8_t *)ota_download_status_buff, sizeof(u8));
	
	os_printf("before:ota_final_buff:0x%x,ota_temp_buff:0x%x,ota_cconfirm_buff:0x%x\r\n",
			ota_final_buff[0],ota_temp_buff[0],ota_cconfirm_buff[0]);
    
	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(bk_ptr->partition_start_addr);
    
	if(offset == OTA_FINA_EXEC_FLAG_OFFSET)
	{
		os_printf("offset == 0\r\n");
		bk_flash_write_bytes((bk_ptr->partition_start_addr + offset),(uint8_t *)&flag, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_TEMP_EXEC_FLAG_OFFSET),(uint8_t *)ota_temp_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_CUSTM_CONF_FLAG_OFFSET),(uint8_t *)ota_cconfirm_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_DOWNLOAD_STATUS_FLAG_OFFSET),(uint8_t *)ota_download_status_buff, sizeof(u8));
	}
	else if(offset == OTA_TEMP_EXEC_FLAG_OFFSET)
	{
		os_printf("offset == 4\r\n");
		bk_flash_write_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_final_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + offset),(uint8_t *)&flag, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_CUSTM_CONF_FLAG_OFFSET),(uint8_t *)ota_cconfirm_buff, sizeof(u8));	
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_DOWNLOAD_STATUS_FLAG_OFFSET),(uint8_t *)ota_download_status_buff, sizeof(u8));
	}
	else if(offset == OTA_CUSTM_CONF_FLAG_OFFSET)
	{
		os_printf("offset == 8\r\n");
		bk_flash_write_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_final_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_TEMP_EXEC_FLAG_OFFSET),(uint8_t *)ota_temp_buff,sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + offset),(uint8_t *)&flag, sizeof(u8));	
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_DOWNLOAD_STATUS_FLAG_OFFSET),(uint8_t *)ota_download_status_buff, sizeof(u8));
	}
	else if(offset == OTA_DOWNLOAD_STATUS_FLAG_OFFSET)
	{
		os_printf("offset ==12\r\n");
		bk_flash_write_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_final_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_TEMP_EXEC_FLAG_OFFSET),(uint8_t *)ota_temp_buff,sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + OTA_CUSTM_CONF_FLAG_OFFSET),(uint8_t *)ota_cconfirm_buff, sizeof(u8));
		bk_flash_write_bytes((bk_ptr->partition_start_addr + offset),(uint8_t *)&flag, sizeof(u8));
	}
	else
	{
		os_printf("input offset value is error !!!! \r\n");
	}
	#if OTA_DEBUG_TEST
    bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_FINA_EXEC_FLAG_OFFSET),(uint8_t *)temp1_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_TEMP_EXEC_FLAG_OFFSET),(uint8_t *)temp2_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_CUSTM_CONF_FLAG_OFFSET),(uint8_t *)temp3_buff, sizeof(u8));
	bk_flash_read_bytes((bk_ptr->partition_start_addr + OTA_DOWNLOAD_STATUS_FLAG_OFFSET),(uint8_t *)temp4_buff, sizeof(u8));
	os_printf("ota_final_buff:0x%x,ota_temp_buff:0x%x,ota_cconfirm_buff:0x%x,ota_download_status_buff:0x%x\r\n",
			temp1_buff[0],temp2_buff[0],temp3_buff[0],temp4_buff[0]);
    #endif
    bk_flash_set_protect_type(protect_type);
}


void bk_ota_confirm_update_partition(ota_confirm_flag ota_confirm_val)
{
    exec_flag ota_exec_flag ;
    uint8 last_exec_flag;
    bk_logic_partition_t *bk_ptr = NULL;

    bk_ptr = bk_flash_partition_get_info(BK_PARTITION_OTA_FINA_EXECUTIVE);
    bk_flash_read_bytes((bk_ptr->partition_start_addr + 8) ,(uint8_t *)&last_exec_flag, sizeof(u8));
    os_printf("bk_ptr->partition_start_addr:0x%x,last_exec_flag:0x%x\r\n",bk_ptr->partition_start_addr,last_exec_flag);

    if(last_exec_flag != ota_confirm_val)
    {
        ota_write_flash(BK_PARTITION_OTA_FINA_EXECUTIVE, ota_confirm_val,8);

        if(ota_confirm_val == CONFIRM_EXEC_A)
        {
            ota_exec_flag = EXEX_A_PART;
            ota_write_flash(BK_PARTITION_OTA_FINA_EXECUTIVE, ota_exec_flag, 0);
        }
        else if(ota_confirm_val == CONFIRM_EXEC_B)
        {
            ota_exec_flag = EXEC_B_PART;
            ota_write_flash(BK_PARTITION_OTA_FINA_EXECUTIVE, ota_exec_flag, 0);
        }
    }
}

static callback_func update_state_cb = NULL;
void bk_ota_register_temp_partition_callback(callback_func cb)
{
    update_state_cb = cb;
}

uint8 ota_temp_execute_partition(int state_val)
{
    uint8 temp_exec_flag = 6;

    if(update_state_cb)
    {
         temp_exec_flag = update_state_cb(state_val);
    }
    
    return temp_exec_flag;
}

#endif // CONFIG_HTTP_AB_PARTITION
u8  ota_flag =0;
#if CONFIG_OTA_HTTP
int bk_http_ota_download(const char *uri)
{
	int ret;
	httpclient_t httpclient;
	httpclient_data_t httpclient_data;
	char http_content[HTTP_RESP_CONTENT_LEN];

	if(!uri)
	{
		ret = BK_FAIL;
		CLI_LOGI( "uri is NULL\r\n");
        
		return ret;
	}
    CLI_LOGI("http_ota_download :0x%x",bk_http_ota_download);

    //__asm volatile ("j .");
#ifdef CONFIG_HTTP_AB_PARTITION
    ota_temp_exec_flag temp_exec_flag ;
    exec_flag exec_temp_part;

    ret = bk_ota_get_current_partition();
    os_printf("ret :0x%x \r\n",ret);
    if(ret == EXEC_B_PART)
    {
		CLI_LOGI("456\r\n");
        update_part_flag = UPDATE_A_PART;
    }
    else
    {
		CLI_LOGI("123\r\n");
        update_part_flag = UPDATE_B_PART;
    }
#endif

#if CONFIG_SYSTEM_CTRL
	bk_wifi_ota_dtim(1);
#endif

	ota_flag = 1;
	os_memset(&httpclient, 0, sizeof(httpclient_t));
	os_memset(&httpclient_data, 0, sizeof(httpclient_data));
	os_memset(&http_content, 0, sizeof(HTTP_RESP_CONTENT_LEN));
	httpclient.header = "Accept: text/xml,text/html,\r\n";
	httpclient_data.response_buf = http_content;
	httpclient_data.response_content_len = HTTP_RESP_CONTENT_LEN;
	ret = httpclient_common(&httpclient,
							uri,
							80,/*port*/
							NULL,
							HTTPCLIENT_GET,
							180000,
							&httpclient_data);


        ota_flag = 0;
	if (0 != ret)
        {
            CLI_LOGI("request epoch time from remote server failed.ret:%d\r\n",ret);
#if CONFIG_SYSTEM_CTRL
	    bk_wifi_ota_dtim(0);
#endif
        }
	else
        {
	    CLI_LOGI("sucess.\r\n");
#ifdef CONFIG_HTTP_AB_PARTITION
	#ifdef CONFIG_OTA_UPDATE_DEFAULT_PARTITION
		#if CONFIG_OTA_POSITION_INDEPENDENT_AB
        	if(update_part_flag == UPDATE_A_PART)
        	{
        		CLI_LOGI("update part A\r\n");
				temp_exec_flag = CONFIRM_EXEC_A; //update A Partition;
        	}
        	else
        	{
        	    CLI_LOGI("update part B\r\n");
				temp_exec_flag = CONFIRM_EXEC_B; //update B Partition;
        	}
		#else
            #ifdef CONFIG_OTA_UPDATE_B_PARTITION
                temp_exec_flag = CONFIRM_EXEC_B; //update B Partition;
            #else
                temp_exec_flag = CONFIRM_EXEC_A; //update A Partition;
            #endif
		#endif
	#else
        temp_exec_flag = ota_temp_execute_partition(ret); //temp_exec_flag :3 :A ,4:B
	#endif
        os_printf("from cus temp_exec_flag:0x%x \r\n",temp_exec_flag);

        if(temp_exec_flag == CONFIRM_EXEC_A)
        {
            os_printf("B>>>A \r\n");
            exec_temp_part = EXEX_A_PART;
        }
        else if(temp_exec_flag == CONFIRM_EXEC_B)
        {
            os_printf("A>>B \r\n");
            exec_temp_part = EXEC_B_PART;
        }
        os_printf("temp_exec_flag:0x%x \r\n",exec_temp_part);
		ota_write_flash(BK_PARTITION_OTA_FINA_EXECUTIVE, exec_temp_part, 4);
		bk_reboot();
#else
		bk_reboot();
#endif /*CONFIG_HTTP_AB_PARTITION*/
	}

	return ret;
}

#endif
