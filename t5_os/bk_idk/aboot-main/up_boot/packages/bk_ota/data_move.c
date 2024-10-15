/*************************************************************
 * @file        data_move.c
 * @brief       data move of ota
 * @author      wifi team
 * @version     V1.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "BK_System.h"
#include "data_move.h"
#include "tiny_aes.h"
#include "zlib.h"
#include "driver_flash.h"
#include "fal_def.h"
#include "fal_cfg.h"
#include "drv_uart.h"
#include "fal.h"

/* buffer use information:
	1, read flash data len:DM_CHUNK, in_buf
	2, aes decrypt len:DM_CHUNK, out_buf
	3, zlib decompress len:DM_CHUNK, in_buf
	4, flash write len: DM_CHUNK * 34 / 32, flash_buf
 */
uint32_t in_buf[DM_CHUNK >> 2];
uint32_t out_buf[DM_CHUNK >> 2];
uint32_t flash_buf[FLASH_WR_CHUNK >> 2];
uint32_t pre_encrypt_buf[FLASH_WR_CHUNK >> 2]; /* efuse encrypt buffer*/
uint8_t remnant[INS_NO_CRC_CHUNK];
uint8_t remnantx[INSTRUCTION_CRC_CHUNK]={0};
uint32_t remnant_len = 0;
uint32_t src_offset1=0;
extern void encrypt(u32 *rx, u8 *tx, u32 num, u32  addr0);
extern void calc_crc(u32 *buf, u32 packet_num);
/**
 * Calculate the CRC32 value of a memory buffer.
 *
 * @param crc accumulated CRC32 value, must be 0 on first call
 * @param buf buffer to calculate CRC32 value for
 * @param len bytes in buffer
 *
 * @return calculated CRC32 value
 */
 static const uint32_t crc32_table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
 /**
 * Calculate the CRC32 value of a memory buffer.
 *
 * @param crc accumulated CRC32 value, must be 0 on first call
 * @param buf buffer to calculate CRC32 value for
 * @param len bytes in buffer
 *
 * @return calculated CRC32 value
 */
uint32_t ota_calc_crc32(uint32_t crc, const void *buf, size_t len)
{
        const uint8_t *p;

        p = (const uint8_t *)buf;
        crc = crc ^ ~0U;
        while (len--) {
            crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ ~0U;
}

/**
 * verify firmware on this partition
 *
 * @param part partition
 *
 * @return -1: failed, >=0: success
 */
int ota_body_fw_verify(const struct fal_partition *part)
{
        struct ota_rbl_hdr hdr;
        uint32_t fw_start_addr, fw_end_addr, i, crc32 = 0;
        uint8_t buf[32], remain_size;

        assert(part);
        if (fal_get_fw_hdr(part->name, &hdr) < 0)
        {
                return -1;
        }
        if (!strcmp(part->name, RT_BK_DL_PART_NAME))
        {
                /* on OTA download partition */
                assert(hdr.size_package >= sizeof(buf));
                
                fw_start_addr = sizeof(struct ota_rbl_hdr);
                fw_end_addr = fw_start_addr + hdr.size_package;
                /* calculate CRC32 */
                for (i = fw_start_addr; i <= fw_end_addr - sizeof(buf); i += sizeof(buf))
                {
                        fal_partition_read(part, i, buf, sizeof(buf));
                        crc32 = ota_calc_crc32(crc32, buf, sizeof(buf));
                }
                /* align process */
                if (i != fw_end_addr - sizeof(buf))
                {
                        remain_size = fw_end_addr - i;
                        fal_partition_read(part, i, buf, remain_size);
                        crc32 = ota_calc_crc32(crc32, buf, remain_size);
                }
                if (crc32 != hdr.crc32)
                {
                bk_printf("Verify firmware CRC32 failed on partition .");
                return -1;
                }
        }   
        bk_printf("Verify ota body partition success.\r\n");
        return 0;
}
void dump_hex(const uint8_t *ptr, size_t buflen)
{
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;

	for (i=0; i<buflen; i+=16)
	{
		printf("%08X: ", i);

		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%02X ", buf[i+j]);
			else
				printf("   ");
		printf(" ");

		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%c", __is_print(buf[i+j]) ? buf[i+j] : '.');
		printf("\n");
	}
}

int aes_decrypt_handler(char *in, char *out, int count,tiny_aes_context ctx,uint8_t *iv)
{
    int len;
    char *buf, *data_decrypt;
    
    buf = in;
    len = count;
    data_decrypt = out;
    memset(data_decrypt, 0x0, len);
    tiny_aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, (unsigned char *)buf, (unsigned char *)data_decrypt);

    return RET_DM_SUCCESS;
}

int data_move_start(fal_partition_t part)
{
#define RD_COUNT    (32)

    int i, ret = 0;
    int count = RD_COUNT;
    int data_0xff_cnt = 0;
    unsigned char rd[RD_COUNT] = {0};

    bk_printf("data_move_start");
    flash_read_data(rd, part->offset, RD_COUNT);
    for(i = 0; i < count; i ++)
    {
        if (0xff == rd[i])
        {
            data_0xff_cnt ++;
        }
		if(0 == i%8)
			bk_printf("\r\n ");
		bk_print_hex(rd[i]);
    }

    if(count == data_0xff_cnt)
    {
        ret = 1;
    }

    return ret;
}



int data_move_end(uint32_t addr)
{
	log_i("data_move_end:%d", DL_SEC_BASE);
    flash_erase_sector(addr);
	
    return 0;
}

int dm_erase_dest_partition(uint32_t addr, uint32_t len)
{
    ASSERT(0 == (addr & (0x1000 - 1)));
    ASSERT(0 == (len & (0x1000 - 1)));

	log_i("dm_erase_dest_partition:0x%x, %d", addr, len);

    flash_erase(addr, len);
	
    return 0;
}

int dm_rd_src_partition(uint32_t flash_addr, char *buf, int len)
{
	log_i("flash_rd:0x%x,:0x%x, %d", flash_addr, buf, len);
    flash_read_data((unsigned char *)buf, flash_addr, len);

    return len;
}

int dm_stay_unalignment_remant(void *unalign_ptr, uint32_t len)
{
	ASSERT(INS_NO_CRC_CHUNK > len);
	
	memcpy(remnant, unalign_ptr, len);
	remnant_len = len;

	return len;
}

int dm_efuse_enc_buf_convert(uint8_t *remnant_ptr, uint32_t remnant_cnt, void *in, void *out, uint32_t count)
{
	int ret;
	int cell_cnt, i, j;
	char *no_crc_cell;
	char *crc_cell;
	char *input_ptr;
	char *output_ptr;
	uint32_t padding_cnt;
	uint32_t handle_all_cnt;

	handle_all_cnt = remnant_cnt + count;
	ASSERT(0 == (handle_all_cnt & (INS_NO_CRC_CHUNK - 1)));
	cell_cnt = handle_all_cnt / INS_NO_CRC_CHUNK;
	ret = cell_cnt;

	output_ptr = (char *)out;
	input_ptr = (char *)in;
	if(remnant_cnt)
	{
		padding_cnt = INS_NO_CRC_CHUNK - remnant_cnt;
		
		memcpy(output_ptr, remnant_ptr, remnant_cnt);
		memcpy(&output_ptr[remnant_cnt], input_ptr, padding_cnt);
		
		output_ptr[CRC_BYTE0_OFFSET] = 0xFF;
		output_ptr[CRC_BYTE1_OFFSET] = 0xFF;

		/* update in/out pointer*/
		input_ptr = &input_ptr[padding_cnt];
		output_ptr = &output_ptr[INSTRUCTION_CRC_CHUNK];
		cell_cnt -= 1;
	}

	no_crc_cell = (char *)input_ptr;
	crc_cell = (char *)output_ptr;
	for(i = 0; i < cell_cnt; i ++)
	{
		for(j = 0; j < INS_NO_CRC_CHUNK; j ++)
		{
			crc_cell[j] = no_crc_cell[j];
		}
		
		crc_cell[CRC_BYTE0_OFFSET] = 0xFF;
		crc_cell[CRC_BYTE1_OFFSET] = 0xFF;

		no_crc_cell = (char *)((uint32_t)no_crc_cell + INS_NO_CRC_CHUNK);
		crc_cell = (char *)((uint32_t)crc_cell + INSTRUCTION_CRC_CHUNK);
	}

	return ret;
}

/*
	paramter:
		addr: physical flash address;
		ptr: write data pointer
		size: cell length
		nmemb: cell count
	return:
		flash physical address, and it is including crc field;
 */
int dm_fwrite(uint32_t addr, void *ptr, size_t size, size_t nmemb)
{
    int original_len = size * nmemb;
	int handle_count, hdl_in_part_cnt;
	int cell_cnt, remnant_cnt;
	char *content;

	log_i("dm_fwrite:0x%x,:0x%x, %d", addr, ptr, original_len);

	/* handle the buffer, including efuse encryption and instrution crc*/
	handle_count = (remnant_len + original_len) / INS_NO_CRC_CHUNK * INS_NO_CRC_CHUNK;
	remnant_cnt = (remnant_len + original_len) - handle_count;
	hdl_in_part_cnt = handle_count - remnant_len;
	
	cell_cnt = dm_efuse_enc_buf_convert(remnant, remnant_len, ptr, pre_encrypt_buf, hdl_in_part_cnt);
    encrypt((u32 *)pre_encrypt_buf, (u8 *)flash_buf, cell_cnt, addr);
    calc_crc((u32 *)flash_buf, cell_cnt);

	/*write flash, and the data has crc field*/
    flash_write_data((unsigned char *)flash_buf, addr, handle_count * INSTRUCTION_CRC_CHUNK / INS_NO_CRC_CHUNK);

	/*record the unalignment data for next writing*/
	content = (char *)ptr;
	dm_stay_unalignment_remant(&content[hdl_in_part_cnt], remnant_cnt);
	
    return handle_count * INSTRUCTION_CRC_CHUNK / INS_NO_CRC_CHUNK;
}

int dm_fwrite_tail(uint32_t addr)
{
	int padding_len, i;
	uint8_t *padding_ptr;
	
	log_i("dm_fwrite_tail:0x%x,:0x%x, %d", addr, remnant, remnant_len);

	padding_len = INS_NO_CRC_CHUNK - remnant_len;
	padding_ptr = &remnant[remnant_len];
	for(i = 0; i < padding_len; i ++)
	{
		padding_ptr[i] = 0xff;
	}
	
	remnant_len = INS_NO_CRC_CHUNK;
	dm_fwrite(addr, 0, 0, 0);

	return padding_len;
}	
void *mem_cpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;
 
	while (count--)
		*tmp++ = *s++;
	return dest;
}

int data_move_handler(void)
{
        unsigned have;
        z_stream strm;
        uint32_t dest_addr, src_addr;
        int ret, src_offset = 0;
        //int dl_partition_len = 0;
        int dl_valid_data_len = 0;
        //int dl_total_len = 0;
        int rd_len, req_len, wr_ret;
        Bytef *zlib_decompress_ptr;
        struct ota_rbl_hdr  rbl_hdr;
        fal_partition_t dest_part = NULL;
        fal_partition_t src_part  = NULL;       
        tiny_aes_context ctx;
        uint8_t iv[TINY_AES_IV_LEN + 1];
        uint8_t private_key[TINY_AES_KEY_LEN + 1];      
        int handle_count;
        int padding_len;
        char  *padding_ptr = NULL;
        char  *contentx = NULL;
        uint32_t  i ,crc32_ret ,crc32= 0;
      
        clr_flash_protect(); // 4
        src_part  = fal_partition_find(RT_BK_DL_PART_NAME);
        bk_printf("fal_partition_find over: ");    
        if(data_move_start(src_part))
        {
                return RET_DM_NO_OTA_DATA;
        }
        //dl_partition_len = src_part->len;
        //read firmware head from download partition 96 bytes
        fal_get_fw_hdr(RT_BK_DL_PART_NAME, &rbl_hdr);
        dest_part = fal_partition_find(rbl_hdr.name);
        dl_valid_data_len = rbl_hdr.size_package;
         /*ota head crc*/
        flash_read_data(flash_buf, src_part->offset, 96);  
        crc32 =ota_calc_crc32(crc32, flash_buf, 92); 
        if(crc32 != rbl_hdr.info_crc32)
        {
                bk_printf("Verify firmware head CRC32 failed on partition .");
                data_move_end(src_part->offset);
                return  RET_DM_FAILURE ;
        }
        bk_printf("Verify firmware head crc sucess\r\n");    
        crc32_ret =ota_body_fw_verify((fal_partition_t)src_part);  //ota body crc
        if(crc32_ret  != 0)
        {
                bk_printf("Verify firmware body CRC32 failed on partition .");
                data_move_end(src_part->offset);
                return RET_DM_FAILURE;
        }
        dm_erase_dest_partition(dest_part->offset, (dest_part->len + (SM_SECTOR - 1)) / SM_SECTOR * SM_SECTOR);
        dest_addr = dest_part->offset;
        src_addr = src_part->offset+ 0x60; //0x60:rbl head size
        /* init aes_iv_key */
        memcpy(iv, BK_TINY_AES_IV, strlen(BK_TINY_AES_IV));
        iv[sizeof(iv) - 1] = '\0';
        memcpy(private_key, BK_TINY_AES_KEY, strlen(BK_TINY_AES_KEY));
        private_key[sizeof(private_key) - 1] = '\0';
        tiny_aes_setkey_dec(&ctx, (uint8_t *) private_key, 256);
        /* allocate inflate state */
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;

        ret = inflateInit(&strm);
        if (ret != Z_OK)
            return ret;
        while((dl_valid_data_len > src_offset) && (ret != Z_STREAM_END))
        {
                req_len = MIN(DM_CHUNK, (dl_valid_data_len - src_offset));
                rd_len = dm_rd_src_partition(src_addr, (char *)in_buf, req_len);
                ASSERT(rd_len == req_len);
                ASSERT(0 == (rd_len & (32 - 1)));
                src_addr += rd_len;
                src_offset += rd_len;
                /* aes_decrypt */
                aes_decrypt_handler((char *)in_buf, (char *)out_buf, rd_len,ctx,iv);
                strm.avail_in = rd_len;
                if (strm.avail_in == 0)
                        break;
                strm.next_in = (Bytef *)out_buf;
                zlib_decompress_ptr = (Bytef *)in_buf;
            /* run inflate() on input until output buffer not full */
                do
                {
                        strm.avail_out = ZLIB_CHUNK;
                        strm.next_out = zlib_decompress_ptr;
#if CFG_BEKEN_OTA
                if(0xaa > strm.avail_in)
                {
                        dump_hex((uint8_t *)strm.next_in, strm.avail_in);
                }
#endif
                ret = inflate(&strm, Z_NO_FLUSH);
                ASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */
                bk_printf("inflate ret");
                bk_print_hex(ret);
                bk_printf("\r\n:");
                switch (ret)
                {
                        case Z_NEED_DICT:
                                ret = Z_DATA_ERROR;     /* and fall through */
                        case Z_DATA_ERROR:
                        case Z_MEM_ERROR:
                                bk_printf("inflateEnd ret");
                                bk_print_hex(ret);
                                bk_printf("\r\n:");
                                (void)inflateEnd(&strm);
                                return ret;
                }
                have = ZLIB_CHUNK - strm.avail_out;
                bk_printf("have:");
                bk_print_hex(have);
                bk_printf("\r\n:");

#if SPI_OPREATE_FLASH
                wr_ret = dm_fwrite(dest_addr, zlib_decompress_ptr, 1, have);
                dest_addr += wr_ret;
#endif
                
#if CPU_OPREATE_FLASH   //cpu write data start           
                REG_FLASH_CONFIG  |= FLASH_CONFIG_FWREN_FLASH_CPU_MASK; //cpu write flash enable       
                contentx = (char *)&in_buf[0];    
                padding_len = INS_NO_CRC_CHUNK - remnant_len;
                if(remnant_len != 0)
                {
                        mem_cpy(&remnant[remnant_len],&contentx[0],padding_len);
                        mem_cpy((USER_APP_ENTRY+src_offset1), &remnant[0], INS_NO_CRC_CHUNK);        
                        src_offset1 += 32;
                        handle_count = have -(padding_len)- ((have - (padding_len))%32);
                        mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[(padding_len)], handle_count);            
                        src_offset1 += handle_count;
                        mem_cpy(&remnant[0],&contentx[(handle_count+padding_len)],(have-handle_count-padding_len));
                        remnant_len = have-handle_count-padding_len;
                }
                else
                {
                        if(have%32 ==0)
                        {
                                mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[0], have);
                                src_offset1 += have;
                        }
                        else
                        {
                                handle_count = have - (have)%32;
                                mem_cpy((USER_APP_ENTRY+src_offset1), &contentx[0], handle_count);
                                src_offset1 += handle_count;                   
                                mem_cpy(&remnant[0],&contentx[handle_count],have-handle_count);

                                remnant_len = have-handle_count;                   
                        }
                }
#endif            		
                }while (strm.avail_out == 0);
        //__asm volatile ("j .");      
                /* done when inflate() says it's done */
        }
#if SPI_OPREATE_FLASH     
        if(remnant_len)
        {
                bk_printf(" 123456\r\n:");
                dm_fwrite_tail(dest_addr);
        }
        ASSERT(0 == remnant_len);
#endif        
#if CPU_OPREATE_FLASH  
        if(remnant_len != 0)
        {	
                padding_len = INS_NO_CRC_CHUNK - remnant_len;
                padding_ptr = &remnant[remnant_len];

                for(i = 0; i < padding_len; i ++)
                {
                        padding_ptr[i] = 0xff;
                }
                mem_cpy(&remnant[remnant_len],&padding_ptr[0],padding_len);

                mem_cpy((USER_APP_ENTRY+src_offset1), &remnant[0], INS_NO_CRC_CHUNK);

                remnant_len = 0;
        }//cpu write data end
        REG_READ(0x0);        
        REG_FLASH_CONFIG  &= (~(FLASH_CONFIG_FWREN_FLASH_CPU_MASK)); //cpu write flash disable
#endif
        /* clean up and return */
        (void)inflateEnd(&strm);
        //erase dl partition
        src_part  = fal_partition_find(RT_BK_DL_PART_NAME);
        data_move_end(src_part->offset);
        return ret == Z_STREAM_END ? RET_DM_SUCCESS : RET_DM_DATA_ERROR;
}
// eof

