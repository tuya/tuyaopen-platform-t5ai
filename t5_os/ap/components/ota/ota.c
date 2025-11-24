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
#ifdef CONFIG_OTA_HASH_FUNCTION
#include "driver/flash_partition.h"
#endif
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"

#ifdef CONFIG_HTTP_AB_PARTITION
#define OTA_DEBUG_TEST                    (0)
#define FLASH_BASE_ADDRESS                (0x44030000)
#define FLASH_OFFSET_ENABLE               (0x19)
#define OTA_FINA_EXEC_FLAG_OFFSET         (0x0)
#define OTA_TEMP_EXEC_FLAG_OFFSET         (0x4)
#define OTA_CUSTM_CONF_FLAG_OFFSET        (0x8)
#define OTA_DOWNLOAD_STATUS_FLAG_OFFSET   (0xC) //0xFE.download start. //0xFC download success
#define FLASH_DEFAULT_VALUE               (0xFFFFFFFF)

part_flag update_part_flag;

#if CONFIG_OTA_POSITION_INDEPENDENT_AB
static uint8 ota_get_flash_offset_enable_value(void)
{
	uint8 ret_val;

	ret_val = (REG_READ((FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4)) & 0x1);
	OTA_LOGD("ret_val  :0x%x\r\n",ret_val);

	return ret_val;
}

void bk_ota_double_check_for_execution(void)
{
	uint8 ret;
	
	ret = bk_ota_get_current_partition();
	if(ret == 0)
	{
		OTA_LOGD("confirm exec pos_ind part a \r\n");
		bk_ota_confirm_update_partition(CONFIRM_EXEC_A);
	}
	else
	{
		OTA_LOGD("confirm exec pos_ind part b \r\n");
		bk_ota_confirm_update_partition(CONFIRM_EXEC_B);
	}
}
#endif

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
	OTA_LOGD("bk_ptr->partition_start_addr  :0x%x\r\n",bk_ptr->partition_start_addr);
	bk_flash_read_bytes(bk_ptr->partition_start_addr ,(uint8_t *)&ota_exec_flag, sizeof(u8));

	return ota_exec_flag ;  // ota_exec_flag :0x0/0xFF represents A 0x1 :represents B 

#endif
}

void ota_write_flash(bk_partition_t ota_partition_flag, u8 flag, u8 offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 ota_flag_buff[16] = {0};
#if OTA_DEBUG_TEST
	u8 ota_temp_buff[16] = {0};
#endif

	flash_protect_type_t protect_type;
	bk_ptr = bk_flash_partition_get_info(ota_partition_flag);
	OTA_LOGV("ota_write_flash:partition_start_addr:0x%x--0x%x\r\n",(bk_ptr->partition_start_addr) ,sizeof(ota_flag_buff));

	os_memset(ota_flag_buff, 0xFF, sizeof(ota_flag_buff));
	bk_flash_read_bytes((bk_ptr->partition_start_addr), (uint8_t *)ota_flag_buff, sizeof(ota_flag_buff));
	OTA_LOGV("before:ota_final_buff:0x%x,ota_temp_buff:0x%x,ota_cconfirm_buff:0x%x\r\n",
		ota_flag_buff[0],ota_flag_buff[4],ota_flag_buff[8]);
    
	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(bk_ptr->partition_start_addr);
    
	switch (offset)
	{
		case OTA_FINA_EXEC_FLAG_OFFSET:
		case OTA_TEMP_EXEC_FLAG_OFFSET:
		case OTA_CUSTM_CONF_FLAG_OFFSET:
		case OTA_DOWNLOAD_STATUS_FLAG_OFFSET:
			os_memset(&ota_flag_buff[offset], flag, sizeof(u8));
			bk_flash_write_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_flag_buff, sizeof(ota_flag_buff));
			break;
		default:
			OTA_LOGE("input offset value is error !!!! \r\n");
			break;
	}

	#if OTA_DEBUG_TEST
	os_memset(ota_temp_buff, 0xFF, sizeof(ota_temp_buff));
	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)ota_temp_buff, sizeof(ota_temp_buff));
	OTA_LOGV("ota_final_buff:0x%x,ota_temp_buff:0x%x,ota_cconfirm_buff:0x%x,ota_download_status_buff:0x%x\r\n",
		ota_temp_buff[0],ota_temp_buff[4],temp3_buff[8],temp4_buff[12]);
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
    OTA_LOGD("bk_ptr->partition_start_addr:0x%x,last_exec_flag:0x%x\r\n",bk_ptr->partition_start_addr,last_exec_flag);

	if(last_exec_flag == 1)	//only after do ota , do double check 
	{
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
}

static callback_func update_state_cb = NULL;
void bk_ota_register_temp_partition_callback(callback_func cb)
{
    update_state_cb = cb;
}

uint8_t ota_temp_execute_partition(int state_val)
{
    uint8_t temp_exec_flag = 6;

    if(update_state_cb)
    {
         temp_exec_flag = update_state_cb(state_val);
    }
    
    return temp_exec_flag;
}

#ifdef CONFIG_OTA_HASH_FUNCTION
int32_t ota_do_hash_check(void)
{
	struct ota_rbl_head  rbl_hdr;
	const bk_logic_partition_t *bk_ptr = NULL;
	uint32_t partition_length = 0;
	int ret = BK_FAIL;
	if(update_part_flag == UPDATE_B_PART) {
		bk_ptr = bk_flash_partition_get_info(BK_PARTITION_S_APP);   //note: when update_partition is B, arg: BK_PARTITION_APPLICATION1,update_partition is A??arg: BK_PARTITION_APPLICATION
	}
	else{    //B-->A
		bk_ptr = bk_flash_partition_get_info(BK_PARTITION_APPLICATION);
	}
	partition_length = bk_flash_partition_get_info(BK_PARTITION_S_APP)->partition_length;
	OTA_LOGV("partition_length :0x%x",partition_length);
	if((bk_ptr == NULL))
	{
		OTA_LOGE(" get %s fail \r\n",bk_ptr->partition_owner);
		return BK_FAIL; 
	}

	ota_get_rbl_head(bk_ptr, &rbl_hdr, partition_length);
	
	ret = ota_hash_verify(bk_ptr, &rbl_hdr);
	if(ret == BK_OK)
	{
		OTA_LOGD("hash sucess!!!! \r\n");
	}

	return ret;
}
#endif

void ota_write_flash_with_all_flag(bk_partition_t ota_partition_flag, uint8_t *p_flag_buf, uint8_t offset, uint8_t size)
{
	bk_logic_partition_t *bk_ptr = NULL;
	flash_protect_type_t protect_type;

	bk_ptr = bk_flash_partition_get_info(ota_partition_flag);
	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(bk_ptr->partition_start_addr + offset);
	bk_flash_write_bytes((bk_ptr->partition_start_addr + offset), (uint8_t *)p_flag_buf, size);
	bk_flash_set_protect_type(protect_type);
}

int bk_ota_update_partition_flag(int input_val)
{
	uint8_t                      cust_confirm_flag;
	exec_flag                   ota_exec_flag;
	exec_flag                   exec_temp_part;
	ota_temp_exec_flag   temp_exec_flag ;

	if(update_part_flag == UPDATE_B_PART){
		ota_exec_flag = EXEC_B_PART;
	}else{
		ota_exec_flag = EXEX_A_PART;
	}
#ifdef CONFIG_OTA_UPDATE_DEFAULT_PARTITION
#if CONFIG_OTA_POSITION_INDEPENDENT_AB
    	if(update_part_flag == UPDATE_A_PART){
		temp_exec_flag = CONFIRM_EXEC_A; //update A Partition;
    	}else{
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
	temp_exec_flag = ota_temp_execute_partition(input_val); //temp_exec_flag :3 :A ,4:B
#endif
	OTA_LOGV("from cus temp_exec_flag:0x%x \r\n",temp_exec_flag);
	if(temp_exec_flag == CONFIRM_EXEC_A){
		OTA_LOGD("B>>A \r\n");
		exec_temp_part = EXEX_A_PART;
	}else if(temp_exec_flag == CONFIRM_EXEC_B){
		OTA_LOGD("A>>B \r\n");
		exec_temp_part = EXEC_B_PART;
	}else{
		OTA_LOGE("temp_exec_flag is error \r\n");
		return BK_FAIL;
	}

 	cust_confirm_flag= 0x1;  //represent do ota update.
#if CONFIG_OTA_EVADE_METHOD
	uint8_t download_status_flag = DOWNLOAD_SUCCESS_FLAG;
#endif
	OTA_LOGD("ota_exec_flag :0x%x, exec_temp_part :0x%x,cust_confirm_flag :0x%x ,download_status_flag :0x%x\r\n",\
		ota_exec_flag ,exec_temp_part, cust_confirm_flag,download_status_flag);

	uint8_t input_flag_buf[16]= {0};  //{ota_exec_flag,0xFF,0xFF,0xFF,exec_temp_part,0xFF,0xFF,0xFF,cust_confirm_flag,0,0,0,download_status_flag,0,0,0};
	os_memset(&input_flag_buf[0], 0xFF, sizeof(input_flag_buf));
	os_memcpy(&input_flag_buf[0], &ota_exec_flag, 1);
	os_memcpy(&input_flag_buf[4], &exec_temp_part, 1);
	os_memcpy(&input_flag_buf[8], &cust_confirm_flag, 1);
	os_memcpy(&input_flag_buf[12], &download_status_flag, 1);
	ota_write_flash_with_all_flag(BK_PARTITION_OTA_FINA_EXECUTIVE, input_flag_buf, 0, 16);

	return BK_OK;
}

int bk_ota_swap_execute_partition(void)
{
	uint8_t     ota_flag_buf[16]= {0};
	uint32_t    a_app_head= 0;
	uint32_t    b_app_head= 0;
	bk_logic_partition_t *bk_ota_ptr = NULL;
	bk_logic_partition_t *a_app_ptr  = NULL;
	bk_logic_partition_t *b_app_ptr  = NULL;

	a_app_ptr = bk_flash_partition_get_info(BK_PARTITION_APPLICATION);
	b_app_ptr = bk_flash_partition_get_info(BK_PARTITION_S_APP);
	bk_ota_ptr = bk_flash_partition_get_info(BK_PARTITION_OTA_FINA_EXECUTIVE);
	if((bk_ota_ptr == NULL)||(a_app_ptr == NULL) ||(b_app_ptr == NULL))
	{
		BK_LOGD(NULL, "get partition fail! \r\n");
		return BK_FAIL;
	}
	bk_flash_read_bytes((a_app_ptr->partition_start_addr), (uint8_t *)&a_app_head, sizeof(uint32_t));
	bk_flash_read_bytes((b_app_ptr->partition_start_addr), (uint8_t *)&b_app_head, sizeof(uint32_t));
	//BK_LOGD(NULL, "a_app_head :0x%x ,b_app_head :0x%x \r\n", a_app_head, b_app_head);
    if ((a_app_head == FLASH_DEFAULT_VALUE)||(b_app_head == FLASH_DEFAULT_VALUE))
	{
		BK_LOGD(NULL, "only one execute partition and forbid swap! \r\n");
		return BK_FAIL;
	}
	else
	{
		flash_protect_type_t protect_type = bk_flash_get_protect_type();
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		exec_flag ret = bk_ota_get_current_partition();
		if(ret == EXEX_A_PART)        //execute A 
		{
			BK_LOGD(NULL, "execute A, swap to B \r\n");
			bk_flash_erase_sector(bk_ota_ptr->partition_start_addr);
			exec_flag   ota_exec_flag = EXEC_B_PART;
			os_memset(&ota_flag_buf[0], 0xFF, sizeof(ota_flag_buf));
			os_memcpy(&ota_flag_buf[0], &ota_exec_flag, 1);
			bk_flash_write_bytes(bk_ota_ptr->partition_start_addr, ota_flag_buf, sizeof(ota_flag_buf));
		}
		else if(ret == EXEC_B_PART)    //execute B
		{
			BK_LOGD(NULL, "execute B, swap to A \r\n");
			bk_flash_erase_sector(bk_ota_ptr->partition_start_addr);
		}
		else
		{
			BK_LOGD(NULL, "swap partition fail! \r\n");
		}
		bk_flash_set_protect_type(protect_type);

		return BK_OK;
	}
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

	if(!uri){
		ret = BK_FAIL;
		OTA_LOGE( "uri is NULL\r\n");
		return ret;
	}
    OTA_LOGV("http_ota_download :0x%x",bk_http_ota_download);
#ifdef CONFIG_HTTP_AB_PARTITION
    ret = bk_ota_get_current_partition();
    OTA_LOGD("ret :0x%x \r\n",ret);
    if(ret == EXEC_B_PART){
        update_part_flag = UPDATE_A_PART;
    } else{
        update_part_flag = UPDATE_B_PART;
    }
#endif

#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(1);//ap do not have this api temporiary
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
							300000,
							&httpclient_data);


	ota_flag = 0;
	if (0 != ret){
		OTA_LOGE("request epoch time from remote server failed.ret:%d\r\n",ret);
#if CONFIG_SYSTEM_CTRL
		//bk_wifi_ota_dtim(0);	//ap do not have this api temporiary
#endif
	}else{

#ifdef CONFIG_HTTP_AB_PARTITION
		int ret_val = 0;
		#ifdef CONFIG_OTA_HASH_FUNCTION
		ret_val= ota_do_hash_check();
		if(ret_val != BK_OK)
		{
			OTA_LOGE("hash fail.\r\n");
			return  ret_val;
		}
		#endif
		ret_val = bk_ota_update_partition_flag(ret);
		if(ret_val != BK_OK)
		{
			return ret_val;
		}
		OTA_LOGI("success.\r\n");
		bk_reboot();
#else
		OTA_LOGI("success.\r\n");
		bk_reboot();
#endif /*CONFIG_HTTP_AB_PARTITION*/
	}
	return ret;
}

#endif
