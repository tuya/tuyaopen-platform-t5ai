#include "bsdiff_flash.h"
#include "ty_bsdiff_adapt.h"
#include "diff2ya.h"
#define BLKSZ(a) (a->cfg.blocksz)

extern uint32_t  flash_crc32_cal(uint32_t addr, uint32_t size, void* arg);

static uint32_t _cal_crc32(uint8_t *buf, uint32_t len)
{
    uint32_t i, crc32= 0;
    for(i = 0; i < len; i++) {
        crc32 += buf[i];
    }

    return crc32;
}

int update_manage_info(manage_info_t *mag_info, flash_info_t *flh_info, void* arg)
{
    if((NULL == mag_info) || (NULL == flh_info))
        return BD_INVALID_PRM;
    
    if(mag_info->magic != BSDF_MAGIC) 
        return BD_CHECK_ERR;
    
    mag_info->blockid = mag_info->blockid ? 0 : 1;
    mag_info->wrcnt++;

    uint8_t *buf = (uint8_t *)mag_info;
    buf += 8;
    uint32_t len = sizeof(manage_info_t) - 8;
    mag_info->crc32 = _cal_crc32(buf, len);

    //erase flash
    uint32_t addr = flh_info->magaddr + mag_info->blockid * BLKSZ(flh_info);
    int ret = ty_adapt_flash_erase(addr, BLKSZ(flh_info), arg);
    if(ret) return ret;

    //write flash
    buf = (uint8_t *)mag_info;
    return ty_adapt_flash_write(addr, buf, sizeof(manage_info_t), arg);
}

//call ends at differential upgrade 
int bsdiff_mag_clear(manage_info_t *mag_info, flash_info_t *flh_info, void* arg)
{
    if((NULL == mag_info) || (NULL == flh_info))
        return BD_INVALID_PRM;

    uint8_t id = (mag_info->blockid) ? 0 : 1;
    uint32_t addr = flh_info->magaddr + id * BLKSZ(flh_info);
    ty_adapt_print("11111111\r\n");
    int ret = ty_adapt_flash_erase(addr, BLKSZ(flh_info), arg);
    ty_adapt_print("22222222\r\n");
    if(ret) return ret;

    id = (id) ? 0 : 1;
    addr = flh_info->magaddr + id * BLKSZ(flh_info);
    
    return ty_adapt_flash_erase(addr, BLKSZ(flh_info), arg);  
}


static int __read_manage_block(flash_info_t *flh_info, uint8_t id, uint8_t *buf, uint32_t len, void* arg) 
{
    if((NULL == flh_info) || (NULL == buf))
        return BD_INVALID_PRM;

    if((id > 1) || (len > BLKSZ(flh_info)))
        return BD_INVALID_PRM;
    ty_adapt_print("flh_info->magaddr:0x%x, id:%d, block:%d\r\n", flh_info->magaddr, id, BLKSZ(flh_info));
    uint32_t addr = flh_info->magaddr + id * BLKSZ(flh_info);
    int ret = ty_adapt_flash_read(addr, buf, len, arg);
    if(ret) {
        ty_adapt_print("ty_adapt_flash_read fail:%d, addr:0x%x, len:%d, buf：%p", ret, addr, len, buf);
        return ret;
    }
    

    //check validity
    uint32_t rescrc = 0;
    manage_info_t *mag = (manage_info_t *)buf;
    if((mag->magic != BSDF_MAGIC) || \
       ((rescrc = _cal_crc32(buf + 8, len - 8)) != mag->crc32) || \
       (mag->blockid != id) || \
       ((mag->state != BSDIFF_RESTORE0) && (mag->state != BSDIFF_RESTORE1) &&(mag->state != BSDIFF_FLIT))) {
        ty_adapt_print("__read_manage_block: mag->blockid=%d, id=%d, mag->state=%d.\r\n", 
                        mag->blockid, id, mag->state);
        ty_adapt_print("__read_manage_block: mag->magic=0x%08x, rescrc=0x%08x, mag->crc32=0x%08x.\r\n", 
                        mag->magic, rescrc, mag->crc32);
        return BD_CHECK_ERR;
    }
        
    return BD_OK;
}


//初始化调用，return 0 -- 需要做差分还原
static int _judge_ota_info(manage_info_t *mag_info, flash_info_t *flh_info, void* arg)
{
    if((NULL == mag_info) || (NULL == flh_info))
        return BD_INVALID_PRM;
    uint8_t *buf0 = ty_adapt_malloc(sizeof(manage_info_t));
    if(NULL == buf0) return BD_MALLOC_ERR;
    uint8_t *buf1 = ty_adapt_malloc(sizeof(manage_info_t));
    if(NULL == buf1) { ty_adapt_free(buf0); return BD_MALLOC_ERR; }
    uint8_t checkerr0 = 0, checkerr1 = 0;
    int ret = __read_manage_block(flh_info, 0, buf0, sizeof(manage_info_t), arg);
    if(ret) {
        if(ret != BD_CHECK_ERR) 
            goto exit;
        else 
            checkerr0 = 1;
    }

    ret = __read_manage_block(flh_info, 1, buf1, sizeof(manage_info_t), arg);
    if(ret) {
        if(ret != BD_CHECK_ERR) 
            goto exit;
        else 
            checkerr1 = 1;
    }

    ty_adapt_print("_judge_ota_info checkerr0:%d, checkerr1%d\r\n", checkerr0,checkerr1);
    ret = BD_OK;
    if(checkerr0) {
        if(checkerr1) {
            ret = BD_NO_NEED_OTA;  
        }else {
            memcpy((uint8_t *)mag_info, buf1, sizeof(manage_info_t));
        }
    }else {
        if(checkerr1) { 
            memcpy((uint8_t *)mag_info, buf0, sizeof(manage_info_t)); 
        }else {
            manage_info_t *mag0 = (manage_info_t *)buf0;
            manage_info_t *mag1 = (manage_info_t *)buf1;
            if(mag0->wrcnt < mag1->wrcnt)
                memcpy((uint8_t *)mag_info, buf1, sizeof(manage_info_t));
            else
                memcpy((uint8_t *)mag_info, buf0, sizeof(manage_info_t)); 
        }
    }

    if(ret == 0) {
        ty_adapt_print("mag_info->blockid:%d, mag_info->ctrlid:%d, mag_info->state:%d.\r\n",mag_info->blockid, mag_info->ctrlid, mag_info->state);
    }

exit: 
    ty_adapt_free(buf0);
    ty_adapt_free(buf1);

    return ret;
}

int _flit_data_step(bsdiff_mag_t *mag, uint32_t dst_length, uint32_t fl_offlen, uint32_t buf_size, uint32_t step, void* arg)
{
    uint32_t block_size = ty_adapt_flash_get_cfg()->blocksz;
    int ret = 0, loop_num = 0;
    int copy_step = 0;
    uint32_t offlen = 0;
    uint32_t crc32_result = 0, crc32_result_up = 0, cntsz = 0;
    flash_info_t *flh_info = &mag->flh_info;
    manage_info_t *mag_info = &mag->mag_info;
    //flash_cfg_t* flash_cfg = ty_adapt_flash_get_cfg();

    mag_info->state=BSDIFF_FLIT;
    
    copy_step = (fl_offlen%block_size == 0) ? fl_offlen : ((fl_offlen/block_size)*block_size);
    copy_step = (copy_step == 0) ? fl_offlen : copy_step;
    loop_num = copy_step/buf_size + 1;
    offlen = (loop_num == 1) ? copy_step : buf_size;
    
    
    uint8_t *buff = ty_adapt_malloc(offlen);
    if(NULL == buff) {
        ty_adapt_print("offlen:%d, fl_offlen:%d, buf_size:%d, step:%d, dst_length:%d, copy_step:%d, block_size:%d\r\n", offlen, fl_offlen, buf_size, step, dst_length, copy_step, block_size);
        return BD_MALLOC_ERR;
    }
        

    //mag_info->flitoff = (mag_info->flitoff < fl_offlen) ? fl_offlen : mag_info->flitoff;
    
//    for (i = 0;i < loop_num;i++) {
    for(cntsz = mag_info->flitoff; cntsz < dst_length; cntsz += offlen) {
        
        mag_info->flitoff = cntsz;
        ty_adapt_print("coping... loop_num:%d, dst_length:0x%02x, flh_info->frmaddr:0x%02x, offlen:0x%02x, fl_offlen:0x%02x step:0x%02x, copy_step:0x%02x, mag_info->flitoff:0x%02x\r\n", \
            loop_num, dst_length, flh_info->frmaddr, offlen, fl_offlen, step, copy_step, mag_info->flitoff);
        ret = update_manage_info(mag_info, flh_info, arg);
        if(ret!=0) {
            ty_adapt_print("update_manage_info error, ret:%d\r\n", ret);
            ty_adapt_free(buff);
            return ret;
        }
        //return -1;
        ret = ty_adapt_flash_read(flh_info->frmaddr + fl_offlen + cntsz, buff, offlen, arg);
        if(ret!=0) {
            ty_adapt_print("ty_adapt_flash_read error, ret:%d\r\n", ret);
            ty_adapt_free(buff);
            return ret;
        }
        if(cntsz >= (offlen*7)) {
            //return -1;
        }
        
        ret = ty_adapt_flash_erase(flh_info->frmaddr + cntsz, offlen, arg);
        if(ret!=0) {
            ty_adapt_print("ty_adapt_flash_erase error, ret:%d\r\n", ret);
            ty_adapt_free(buff);
            return ret;
        }
            

        ret = ty_adapt_flash_write(flh_info->frmaddr + cntsz, buff, offlen, arg);
        if(ret!=0) {
            ty_adapt_print("ty_adapt_flash_write error, ret:%d\r\n", ret);
            ty_adapt_free(buff);
            return ret;
        }

        if(dst_length < cntsz + offlen) {
            offlen = dst_length - cntsz;
            ty_adapt_print("offlen = %d\r\n", offlen);
        }
        
        crc32_result_up = flash_crc32_cal(flh_info->frmaddr + cntsz, offlen, arg);
        crc32_result = flash_crc32_cal(flh_info->frmaddr + fl_offlen + cntsz, offlen, arg);
        if(crc32_result_up != crc32_result) {
            ty_adapt_print("new_flash_crc32 error, crc32_result_up:%x, crc32_result:%x, offlen:0x%x\r\n", crc32_result_up, crc32_result, offlen);
            ty_adapt_free(buff);
            return DIFF2YA_ERROR_NEW_FLASH_DATA;
        }
    }
 //   }
    
    ty_adapt_free(buff);
    return ret;
}

//flag = 1, finished -- clear manage
int _bkup_2_frmware(bsdiff_mag_t *mag, uint32_t frmaddr, uint8_t *buf, uint32_t len, uint8_t flag, void* arg)
{
    flash_info_t *flh_info = &mag->flh_info;
    manage_info_t *mag_info = &mag->mag_info;

    if(mag_info->state != BSDIFF_RESTORE1) {
        return BD_COM_ERR;
    }
    #if ___DEBUG
    ty_adapt_print("frmaddr1:0x%x, flag:%d, len:0x%x\r\n",frmaddr,flag, len);
    ty_adapt_print("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \r\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
    #endif
    //to do
    int ret = ty_adapt_flash_read(flh_info->bkupaddr, buf, len, arg);
    if(ret) return ret;

    ret = ty_adapt_flash_erase(frmaddr, len, arg);
    if(ret) return ret;

    ret = ty_adapt_flash_write(frmaddr, buf, len, arg);
    if(ret) return ret;


    if(flag) {
        // ret = bsdiff_mag_clear(mag_info, flh_info);
    }else {
        mag_info->state = BSDIFF_RESTORE0;
        ret = update_manage_info(mag_info, flh_info, arg);
    }

    return ret;
}


int _buff_2_bkup_2_frmware(bsdiff_mag_t *mag, uint32_t ctrlid, uint32_t frmaddr, uint8_t *buf, uint32_t len, uint8_t flag, void* arg)
{
    flash_info_t *flh_info = &mag->flh_info;
    manage_info_t *mag_info = &mag->mag_info;

    if(mag_info->state != BSDIFF_RESTORE0) {
        return BD_COM_ERR;
    }

    #if ___DEBUG
    ty_adapt_print("frmaddr:0x%x, flag:%d, len:0x%x\r\n",frmaddr,flag, len);
    ty_adapt_print("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \r\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
    #endif

    int ret = ty_adapt_flash_erase(flh_info->bkupaddr, len, arg);
    if(ret) return ret;

    ret = ty_adapt_flash_write(flh_info->bkupaddr, buf, len, arg);
    if(ret) return ret;

    mag_info->state = BSDIFF_RESTORE1;
    mag_info->ctrlid = ctrlid;
    ret = update_manage_info(mag_info, flh_info, arg);
    if(ret) return ret;

    // static uint8_t cnt = 0;
    // if(cnt++ >= 30) { 
    //     ty_adapt_print("start reset\r\n");
    //     while(1);
    // }

    ret = ty_adapt_flash_erase(frmaddr, len, arg);
    if(ret) return ret;

    ret = ty_adapt_flash_write(frmaddr, buf, len, arg);
    if(ret) return ret;

    if(flag) {
        // ret = bsdiff_mag_clear(mag_info, flh_info);
    }else {
        mag_info->state = BSDIFF_RESTORE0;
        ret = update_manage_info(mag_info, flh_info, arg);
    }

    return ret;
}

#define MAGSZ(cfg) (2 * cfg->blocksz)



int bsdiff_flash_init(manage_info_t *mag_info, flash_info_t *flh_info, diff2ya_header_t *diff2ya_header, void* arg) 
{
    
    flash_cfg_t *cfg = ty_adapt_flash_get_cfg();
    if(NULL == cfg) 
        return BD_COM_ERR;
    memcpy(&flh_info->cfg, cfg, sizeof(flash_cfg_t));
    flh_info->magaddr = cfg->manager_addr;
    int ret = _judge_ota_info(mag_info, flh_info, arg);
    if(ret) return ret;

    flh_info->frmaddr = mag_info->flh_start_addr;
    
    uint32_t sz = mag_info->patchsz;

    if(sz % BLKSZ(flh_info))
        sz += BLKSZ(flh_info) - sz % BLKSZ(flh_info);

    flh_info->pthaddr = (mag_info->is_confirm_addr == 0) ? (flh_info->magaddr - sz) : mag_info->patch_confirm_addr;
    
    if((mag_info->flh_start_addr % cfg->blocksz) || \
       (mag_info->flh_len % cfg->blocksz)) 
       return BD_COM_ERR;

    // load_and_check_fota_header 
    ret = ty_adapt_flash_read(flh_info->pthaddr, (uint8_t *)diff2ya_header, sizeof(diff2ya_header_t), arg);
    if (ret) return ret;

    // info log print
    ty_adapt_print("\r\n***************diff2ya_header info***********************\r\n");
    ty_adapt_print("diff2ya_header.magic_ver = 0x%x\r\n", diff2ya_header->magic_ver);
    ty_adapt_print("diff2ya_header.bin_type = 0x%x\r\n", diff2ya_header->bin_type);
    ty_adapt_print("diff2ya_header.src_crc32 = 0x%x\r\n", diff2ya_header->src_crc32);
    ty_adapt_print("diff2ya_header.dst_crc32 = 0x%x\r\n", diff2ya_header->dst_crc32);
    ty_adapt_print("diff2ya_header.src_length = %d\r\n", diff2ya_header->src_length);
    ty_adapt_print("diff2ya_header.dst_length = %d\r\n", diff2ya_header->dst_length);
    ty_adapt_print("diff2ya_header.bin_offset = %d\r\n", diff2ya_header->bin_offset);
    ty_adapt_print("diff2ya_header.bin_start_addr = 0x%x\r\n", diff2ya_header->bin_start_addr);
    ty_adapt_print("diff2ya_header.bin_length = %d\r\n", diff2ya_header->bin_length);
    ty_adapt_print("diff2ya_header.bin_crc32 = 0x%x\r\n", diff2ya_header->bin_crc32);
    ty_adapt_print("diff2ya_header.step = 0x%x\r\n", diff2ya_header->step);
    ty_adapt_print("diff2ya_header.buf_size = %x\r\n", diff2ya_header->buf_size);
    ty_adapt_print("diff2ya_header.mode = %d\r\n", diff2ya_header->mode);
    ty_adapt_print("diff2ya_header.ver_supp_min_code = %d\r\n", diff2ya_header->ver_supp_min_code);
    ty_adapt_print("*********************************************************\r\n\r\n");

    flash_cfg_t* flash_cfg = ty_adapt_flash_get_cfg();  
    if(SINGLE_PATCH != diff2ya_header->bin_type) {
        ty_adapt_print("patch type err:%d\r\n", diff2ya_header->bin_type);
        return DIFF2YA_ERROR_PATCH_TYPE;
    }

    if(diff2ya_header->ver_supp_min_code > DIFF2YA_MODULE_VER_CODE) {
        ty_adapt_print("diff ota no support this patch, diff ota ver:%d, patch_supp_min_code:%d\r\n", DIFF2YA_MODULE_VER_CODE, diff2ya_header->ver_supp_min_code);
        return DIFF2YA_ERROR_DIFF_VER_NO_SUPP;
    } else {
		ty_adapt_print("diff ota ver:%d, patch_supp_min_code:%d\r\n", DIFF2YA_MODULE_VER_CODE, diff2ya_header->ver_supp_min_code);
	}
    
    if((diff2ya_header->buf_size)%flash_cfg->blocksz != 0) {
        ty_adapt_print("buf_size err:%x\r\n", diff2ya_header->buf_size);
        return DIFF2YA_ERROR_BUF_SIZE;
    }
    
    flh_info->bkupaddr = (mag_info->is_confirm_addr == 0) ? (flh_info->pthaddr - diff2ya_header->buf_size) : mag_info->backup_confirm_addr;

    ty_adapt_print("mag_info->magic: 0x%x\r\n",mag_info->magic);
    ty_adapt_print("mag_info->crc32: 0x%x\r\n",mag_info->crc32);
    ty_adapt_print("mag_info->patchsz: 0x%x\r\n",mag_info->patchsz);
    ty_adapt_print("mag_info->wrcnt: %d\r\n",mag_info->wrcnt);
    ty_adapt_print("mag_info->ctrlid: %d\r\n",mag_info->ctrlid);
    ty_adapt_print("mag_info->flitoff: 0x%x\r\n",mag_info->flitoff);
    ty_adapt_print("mag_info->blockid: %d\r\n",mag_info->blockid);
    ty_adapt_print("mag_info->state: %d\r\n",mag_info->state);
    ty_adapt_print("mag_info->is_confirm_addr: %d\r\n",mag_info->is_confirm_addr);
    ty_adapt_print("mag_info->flh_start_addr: 0x%x\r\n",mag_info->flh_start_addr);
    ty_adapt_print("mag_info->flh_len: 0x%x\r\n",mag_info->flh_len);
    ty_adapt_print("mag_info->backup_confirm_addr: 0x%x\r\n",mag_info->backup_confirm_addr);
    ty_adapt_print("mag_info->patch_confirm_addr: 0x%x\r\n",mag_info->patch_confirm_addr);

    ty_adapt_print("flh_info->frmaddr: 0x%x\r\n",flh_info->frmaddr);
    ty_adapt_print("flh_info->pthaddr: 0x%x\r\n",flh_info->pthaddr);
    ty_adapt_print("flh_info->bkupaddr: 0x%x\r\n",flh_info->bkupaddr);
    ty_adapt_print("flh_info->magaddr: 0x%x\r\n",flh_info->magaddr);

    uint32_t firm_pack_len = (diff2ya_header->dst_length > diff2ya_header->src_length) ? diff2ya_header->dst_length : diff2ya_header->src_length;
    if (firm_pack_len%cfg->blocksz) {
        firm_pack_len += cfg->blocksz - firm_pack_len%cfg->blocksz;
    }
    uint32_t max_len = firm_pack_len + diff2ya_header->bin_length + diff2ya_header->step + diff2ya_header->buf_size + 2*cfg->blocksz;
    if (max_len%cfg->blocksz) {
        max_len += cfg->blocksz - max_len%cfg->blocksz;
    }
    if(mag_info->flh_len < max_len) {
        ty_adapt_print("max len err..., firm_pack_len:0x%x, max_len:0x%x\r\n", firm_pack_len, max_len);
        return DIFF2YA_ERROR_PATCH_PACKAGE;
    } else {
        ty_adapt_print("max len...firm_pack_len:0x%x, max_len:0x%x\r\n", firm_pack_len, max_len);
    }
    ty_adapt_print("*********************************************************\r\n\r\n");
    return 0;
}



