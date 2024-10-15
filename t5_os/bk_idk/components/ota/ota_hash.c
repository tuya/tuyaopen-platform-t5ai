#include "sdkconfig.h"
#include <string.h>
#include "bk_private/bk_ota_private.h"
#include <os/str.h>
#include <os/mem.h>
#include "cli.h"
#include "driver/flash.h"
#include "vendor_flash_partition.h"

uint32_t fnv1a_r(unsigned char oneByte, uint32_t hash)
{
    return (oneByte ^ hash) * 0x01000193; // 0x01000193 = 16777619
}

/* hash a block of memory */
uint32_t fnv1a(const void* data, uint32_t numBytes, uint32_t hash)
{
    const unsigned char* ptr = (const unsigned char*)data;

    while (numBytes--)
    hash = fnv1a_r(*ptr++, hash);

    return hash;
}

uint32_t ota_calc_hash(uint32_t hash, const void *buf, size_t len)
{
    return fnv1a(buf, len, hash);
}

/**
 * read data from partition
 *
 * @param part partition
 * @param addr relative address for partition
 * @param buf read buffer
 * @param size read size
 *
 * @return >= 0: successful read data size
 *           -1: error
 */

static int32_t ota_read_part_handler(long offset, uint8_t *buf, size_t size)
{
    uint32_t stage_offset = 0;
    uint32_t stag_len = 0;

    stage_offset = offset /32 *32;
    stag_len = size;
    os_memcpy(&buf[0], (void *)stage_offset, stag_len);

    return size;
}


int32_t ota_read_partition(const bk_logic_partition_t *part, uint32_t addr, uint8_t *buf, size_t size, uint32_t offset)
{
        int ret = 0;
 
     	if((part == NULL) ||(buf == NULL)) 
        {
			return BK_FAIL;
        }

        if (addr + size > part->partition_length)
        {
        	OTA_LOGE("read data exceeds the partition size \r\n");
            
            return BK_FAIL;
        }

        ret = ota_read_part_handler(((part->partition_start_addr + offset)*32/34 + addr), buf, size);
 
        if (ret < 0)
        {
        	OTA_LOGE(" ota_read_partition ret :%d\r\n",ret);
        }
        return ret;
}


int32_t ota_get_rbl_head(const bk_logic_partition_t *bk_ptr, struct ota_rbl_head *hdr)
{

    OTA_LOGD("p_start_addr :0x%x,p_length:0x%x, p_name:%s\r\n",bk_ptr->partition_start_addr,bk_ptr->partition_length,bk_ptr->partition_description);
	/* firmware header is on other partition bottom */
   	ota_read_partition(bk_ptr, 0, (uint8_t *)hdr, sizeof(struct ota_rbl_head),(bk_ptr->partition_length -(RBL_HEAD_POS*34/32)));	

	return 0;
}


/**
 * verify firmware hash code on this partition
 *
 * @param part partition @note this partition is not 'OTA download' partition
 * @param hdr firmware header
 *
 * @return -1: failed, >=0: success
 */
int32_t ota_hash_verify(const bk_logic_partition_t *part, const struct ota_rbl_head *hdr)
{
    uint32_t start_addr, end_addr, hash = RT_OTA_HASH_FNV_SEED, i;
    uint8_t buf[32], remain_size; //buf[34],due data is add crc
		
    start_addr = 0;
    end_addr = start_addr + (hdr->size_raw);

    OTA_LOGD("end_addr :0x%x,hdr->size_raw :0x%x,hdr->hash :0x%x\r\n ",end_addr,hdr->size_raw,hdr->hash);

    /* calculate hash */
    for (i = start_addr; i <= end_addr - sizeof(buf); i += sizeof(buf))
    {
        ota_read_partition(part, i, buf, sizeof(buf),0);
        hash = ota_calc_hash(hash, buf, sizeof(buf));
    }

    OTA_LOGI(" i :0x%x,hash :0x%x\r\n ",i,hash);
    /* align process */
    if (i != end_addr - sizeof(buf))
    { 
        remain_size = end_addr - i;
        ota_read_partition(part, i, buf, remain_size, 0);
        hash = ota_calc_hash(hash, buf, remain_size);
        OTA_LOGE(" >>> i :0x%x,hash :0x%x\r\n ",i,hash);
    }
    
    OTA_LOGI("hash sucess!!!! \r\n");

    if (hash != hdr->hash)
    {
        OTA_LOGE("Verify firmware hash(calc.hash: %08lx != hdr.hash: %08lx) failed on partition '%s'.", hash, hdr->hash,part->partition_owner);
        return -1;
    }

    return 0;
}
