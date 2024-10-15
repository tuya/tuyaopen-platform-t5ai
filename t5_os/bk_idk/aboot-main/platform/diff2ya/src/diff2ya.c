/**
 * @file diff2ya.c
 * @brief 
 * @version 0.1
 * @date 2021-3-29
 * 
 * @copyright Copyright Tuya Inc(c) 2019
 * 
 */

#include "diff2ya.h"
#include "xz_read.h"
#include "bspatch.h"
#include "utils_crc32.h"
#include "bsdiff_flash.h"
#include "ty_bsdiff_adapt.h"

#if (BYTE_ORDER == BIG_ENDIAN)
#define bl_swap_32bit(x) ((((x) >> 24) & 0xFF) + (((x) >> 8) & 0xFF00) + (((x) << 8) & 0xFF0000) + ((x) << 24))
#define bl_swap_16bit(x) (((((x) >> 8) & 0xFF) + ((x) << 8)) & 0xFFFF)
void diff2ya_header_big_little_end_swap(diff2ya_header_t* header)
{
    header->magic_ver = bl_swap_32bit(header->magic_ver);
    header->bin_type = bl_swap_32bit(header->bin_type);
    header->src_crc32 = bl_swap_32bit(header->src_crc32);
    header->dst_crc32 = bl_swap_32bit(header->dst_crc32);
    header->src_length = bl_swap_32bit(header->src_length);
    header->dst_length = bl_swap_32bit(header->dst_length);
    header->bin_offset = bl_swap_32bit(header->bin_offset);
    header->bin_start_addr = bl_swap_32bit(header->bin_start_addr);
    header->bin_length = bl_swap_32bit(header->bin_length);
    header->bin_crc32 = bl_swap_32bit(header->bin_crc32);
}
#endif

int xz_patch_read(nb_file_t* stream, void* buffer, uint32_t length, void* arg)
{
    int ret = ty_adapt_flash_read(stream->pos + stream->offset , buffer, length, arg);
    if(ret) return ret;
    stream->pos += length;
    return length;
}

int xz_decode_read(const struct bspatch_stream* stream, void* buffer, int length, void* arg)
{
    return xz_read(stream->opaque, buffer, length, arg);
}
 
uint32_t  flash_crc32_cal(uint32_t addr, uint32_t size, void* arg)
{
    uint32_t read_block = ty_adapt_flash_get_cfg()->blocksz;
    //uint8_t read_buffer[READ_BLOCK];
    // ty_adapt_print("*********0\r\n");
    
    uint8_t *read_buffer = ty_adapt_malloc(read_block);
    if(read_buffer == NULL) {
        ty_adapt_print("malloc error\r\n");
        return 0;
    }
// ty_adapt_print("*********9\r\n");
    uint32_t read_len = 0;
    uint32_t pos = 0;
    
    uint32_t crc32 = hash_crc32i_init();
// ty_adapt_print("*********8\r\n");
    while (1) {
        read_len = size - pos > read_block ? read_block : size - pos;
        // ty_adapt_print("*********7\r\n");
        int ret = ty_adapt_flash_read(addr + pos, read_buffer, read_len, arg);
        // ty_adapt_print("*********6\r\n");
        
        if(ret) {
            // ty_adapt_print("*********5\r\n");
            ty_adapt_free(read_buffer);
            // ty_adapt_print("*********4\r\n");
            return ret;
        }
        
        
        crc32 = hash_crc32i_update(crc32, read_buffer, read_len);
        // ty_adapt_print("*********3\r\n");
        pos += read_len;
        if (pos >= size) {
            break;
        }
    }
    // ty_adapt_print("*********2\r\n");
    
    crc32 = hash_crc32i_finish(crc32);
// ty_adapt_print("*********1\r\n");
    ty_adapt_free(read_buffer);
    // ty_adapt_print("*********11\r\n");
    return crc32;
}


diff2ya_status_t diff2ya_run(bsdiff_mag_t *mag, diff2ya_header_t *diff2ya_header, void* arg)
{
    flash_info_t *flh_info = &mag->flh_info;
    manage_info_t *mag_info = &mag->mag_info;
    uint32_t  crc32_result = 0;
    int bspatch_ret = 0;
    xzread_handler_t xz_handler;
    xzread_handler_t xz_handler1;
    uint32_t offlen = 0;
    flash_cfg_t* flash_cfg = ty_adapt_flash_get_cfg();
    ty_adapt_print("1------------\r\n");
    
    // Verify the old bin crc32
    if(mag_info->ctrlid == 0) {
        crc32_result = flash_crc32_cal(flh_info->frmaddr, diff2ya_header->src_length, arg);
        if (diff2ya_header->src_crc32 != crc32_result) {
            ty_adapt_print("src_crc32 error, src:0x%x, require:0x%x\r\n", crc32_result, diff2ya_header->src_crc32);
            //while(1);
            return DIFF2YA_ERROR_CRC32_OLD;
        }
    }
    
    ty_adapt_print("12------------\r\n");
    // Verify the patch crc32
    uint32_t patch_bin_addr = flh_info->pthaddr + diff2ya_header->bin_offset + diff2ya_header->bin_start_addr;
    ty_adapt_print("patch_bin_addr:0x%x, diff2ya_header->bin_length:%d\r\n", patch_bin_addr, diff2ya_header->bin_length);
    
    crc32_result = flash_crc32_cal(patch_bin_addr, diff2ya_header->bin_length, arg);
    ty_adapt_print("333333333\r\n");
    
    if (diff2ya_header->bin_crc32 != crc32_result) {
        ty_adapt_print("bin_crc32 error, require:0x%x, crc32_result:0x%x.\r\n", diff2ya_header->bin_crc32, crc32_result);
        return DIFF2YA_ERROR_CRC32_PATCH;
    }
    ty_adapt_print("1ty_adapter_get_free_heap_size:%d\r\n", ty_adapter_get_free_heap_size());
    
    ty_adapt_print("mag_info->state:%d\r\n",mag_info->state);
    nb_file_t patch;
    switch(mag_info->state) {
    case BSDIFF_RESTORE0:
    case BSDIFF_RESTORE1:
        // xz package
        patch.offset = flh_info->pthaddr + diff2ya_header->bin_offset + diff2ya_header->bin_start_addr;
        patch.pos = 0;
        patch.size = diff2ya_header->bin_length;
        patch.read = xz_patch_read;

        // xz handler init
        xz_crc32_init();
        
        if (!xz_init(&xz_handler, &patch)) {
            ty_adapt_print("xz_init fail\r\n");
            // while(1);
            return -1;
        }
// ty_adapt_print("122========\r\n");
// while(1);
        bspatch_stream_t bspatch_stream = {
            .opaque = &xz_handler,
            .read = xz_decode_read
        };
// ty_adapt_print("14========\r\n");
// while(1);
        //for extra 
        if (!xz_init(&xz_handler1, &patch)) {
            ty_adapt_print("xz_init1 fail\r\n");
            while(1);
            return -1;
        }
// ty_adapt_print("13========\r\n");
// while(1);
        bspatch_stream_t bspatch_stream1 = {
            .opaque = &xz_handler1,
            .read = xz_decode_read
        };
        // ty_adapt_print("12========\r\n");
// while(1);
        bspatch_ret = bspatch_core( flh_info->frmaddr, diff2ya_header->dst_length, 
                                        flh_info->frmaddr, diff2ya_header->src_length, 
                                        diff2ya_header->mode, diff2ya_header->step, diff2ya_header->buf_size, 
                                        &bspatch_stream, &bspatch_stream1, mag, arg);
        if (bspatch_ret != 0) {
            ty_adapt_print("bspatch_core error %d\r\n",bspatch_ret);
            // while(1);
            return bspatch_ret;
        }
        // ty_adapt_print("========\r\n");
// while(1);
    case BSDIFF_FLIT:
        if(diff2ya_header->mode && (diff2ya_header->dst_length < diff2ya_header->src_length)) {
            uint32_t new_size_align = diff2ya_header->dst_length % flash_cfg->blocksz;
            uint32_t maxcnt = diff2ya_header->src_length + diff2ya_header->step + new_size_align;
            if(maxcnt%flash_cfg->blocksz){
                maxcnt += flash_cfg->blocksz - (maxcnt % flash_cfg->blocksz);
            }
            offlen = maxcnt - diff2ya_header->dst_length;
            if(offlen%flash_cfg->blocksz) {
                offlen -= (offlen%flash_cfg->blocksz);
            }
        } else {
            offlen = diff2ya_header->step;
        }

        if (diff2ya_header->step > 0 || offlen > 0) {
            //向前拷贝step
            bspatch_ret = _flit_data_step(mag, diff2ya_header->dst_length, offlen, diff2ya_header->buf_size, diff2ya_header->step, arg);
            if(0 != bspatch_ret) {
                ty_adapt_print("_flit_data_step error, ret:%d\r\n", bspatch_ret);
                return bspatch_ret;
            }
         }
    }
    
    int cntsz = 0;
    int dlen = diff2ya_header->buf_size;
    for(cntsz = 0; cntsz < diff2ya_header->dst_length; cntsz += dlen) {
        if(diff2ya_header->dst_length < dlen + cntsz){
            dlen = diff2ya_header->dst_length - cntsz;
        }
        crc32_result = flash_crc32_cal(flh_info->frmaddr + cntsz, dlen, arg);
    }

    crc32_result = flash_crc32_cal(flh_info->frmaddr, diff2ya_header->dst_length, arg);
    ty_adapt_print("Verify the new firmware mode:%d, step:%d, offlen:0x%x.\r\n",
                    diff2ya_header->mode, diff2ya_header->step, offlen);
    ty_adapt_print("Verify the new firmware dst_length:0x%x, src_length:0x%x.\r\n",
                    diff2ya_header->dst_length, diff2ya_header->src_length);
    if (diff2ya_header->dst_crc32 != crc32_result) {
        ty_adapt_print("bin_crc32 error, require:0x%x\r\n", diff2ya_header->dst_crc32);
        return DIFF2YA_ERROR_CRC32_NEW;
    }
    
    if(bsdiff_mag_clear(mag_info, flh_info, arg)) {
        ty_adapt_print("clear maneger err...\r\n");
        return DIFF2YA_ERROR_PATCH_PACKAGE;
    }

    ty_adapt_print("#################### FOAT successd! #####################\r\n");
	ty_adapt_print("ty_adapter_get_free_heap_size:%d\r\n", ty_adapter_get_free_heap_size());
    return DIFF2YA_OK;
}


int ty_bsdiff_entry(void* arg)
{
    bsdiff_mag_t mag;
    diff2ya_header_t diff2ya_header;
    ty_adapt_print("-----ty_bsdiff_entry-----\r\n");
    memset(&mag, 0, sizeof(bsdiff_mag_t));
    memset(&diff2ya_header, 0, sizeof(diff2ya_header_t));
    int ret = bsdiff_flash_init(&mag.mag_info, &mag.flh_info, &diff2ya_header, arg);
    if(ret) {
        ty_adapt_print("00------------\r\n");
        goto exit;
    }
    ty_adapt_print("0------------\r\n");
    ret = diff2ya_run(&mag, &diff2ya_header, arg);
    ty_adapt_print("diff2ya_run: %d\r\n", ret);
    // while(1);
    if(ret) {
        goto exit;
    }
    return ret;   

exit:
    if(BD_CHECK_ERR!=ret && BD_NO_NEED_OTA!=ret) {
        ty_adapt_print("mag_clear res:%d\r\n", bsdiff_mag_clear(&mag.mag_info, &mag.flh_info, arg));
    }
    ty_adapt_print("ty_bsdiff_entry run end... ret:%d\r\n", ret);
    // while(1);
    return ret;
}