/*
 * Simple XZ decoder command line tool
 *
 * Author: Lasse Collin <lasse.collin@tukaani.org>
 *
 * This file has been put into the public domain.
 * You can do whatever you want with this file.
 */

/*
 * This is really limited: Not all filters from .xz format are supported,
 * only CRC32 is supported as the integrity check, and decoding of
 * concatenated .xz streams is not supported. Thus, you may want to look
 * at xzdec from XZ Utils if a few KiB bigger tool is not a problem.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bspatch.h"
#include "xz_read.h"
#include "bsdiff_flash.h"
/*
  Patch file format:
  0        8       BSDIFF_CONFIG_MAGIC (see minibsdiff-config.h)
  8        8       X
  16       8       Y
  24       8       sizeof(newfile)
  32       X       control block
  32+X     Y       diff block
  32+X+Y   ???     extra block
  with control block a set of triples (x,y,z) meaning "add x bytes
  from oldfile to x bytes from the diff block; copy y bytes from the
  extra block; seek forwards in oldfile by z bytes".
*/

/* ------------------------------------------------------------------------- */
/* -- Patch file magic number ---------------------------------------------- */

/** MUST be 8 bytes long! */
/** TODO FIXME: we should static_assert this */
#define BSDIFF_CONFIG_MAGIC "DIFF2YA0"
/* ------------------------------------------------------------------------- */
//#define READ_MAX 512
#define READ_MAX 128

#define CTRL_BLOCK_LEN 24
#define BSPATCH_HEADER_LEN 32

typedef struct
{
    int32_t diff;
    int32_t extra;
    int32_t oldnext;
} ctrl_block_t;

typedef struct
{
    ctrl_block_t ctrl;
    uint16_t ctrl_num;
    uint32_t ctrllen;
    uint32_t datalen;
    uint32_t extralen;
    uint32_t newsize;
    uint32_t newpos;
    uint32_t newpos_extra;
    uint32_t oldpos;
} bspatch_handler_t;

static int64_t offtin(uint8_t* buf)
{
    int64_t y;
    y = buf[7] & 0x7F;
    y = y * 256;
    y += buf[6];
    y = y * 256;
    y += buf[5];
    y = y * 256;
    y += buf[4];
    y = y * 256;
    y += buf[3];
    y = y * 256;
    y += buf[2];
    y = y * 256;
    y += buf[1];
    y = y * 256;
    y += buf[0];

    if (buf[7] & 0x80)
        y = -y;

    return y;
}

int bspatch_core(uint32_t new_bin_addr, uint32_t new_size, 
                 uint32_t old_bin_addr, uint32_t old_size, 
                 uint32_t mode, uint32_t step, uint32_t buf_size, 
                 bspatch_stream_t* patch, bspatch_stream_t* patch1, 
                 bsdiff_mag_t *mag,
				 void* arg)

{
    flash_info_t *flh_info = &mag->flh_info;
    manage_info_t *mag_info = &mag->mag_info;
    flash_cfg_t* flash_cfg = ty_adapt_flash_get_cfg();
    ty_adapt_print("ty_adapter_get_free_heap_size:%d\r\n", ty_adapter_get_free_heap_size());

    uint8_t *buff = ty_adapt_malloc(buf_size);
    if(NULL == buff) 
        return BD_MALLOC_ERR;

    uint8_t read_buf[READ_MAX] = {0};
    bspatch_handler_t bspatch;
    memset(&bspatch, 0, sizeof(bspatch_handler_t));
    // ty_adapt_print("9999999\r\n");

    if (patch->read(patch, read_buf, BSPATCH_HEADER_LEN, arg) != BSPATCH_HEADER_LEN) {
        ty_adapt_print("BSPATCH_HEADER_LEN read fail1!\r\n");
        ty_adapt_free(buff);
        return -1;
    }

    /* Make sure magic and header fields are valid */
    if (memcmp(read_buf, BSDIFF_CONFIG_MAGIC, 8) != 0) {
        ty_adapt_free(buff);
        return -1;
    }

    if (patch1->read(patch1, read_buf, BSPATCH_HEADER_LEN, arg) != BSPATCH_HEADER_LEN) {
        ty_adapt_print("BSPATCH_HEADER_LEN read fail2!\r\n");
        ty_adapt_free(buff);
        return -1;
    }

    /* Make sure magic and header fields are valid */
    if (memcmp(read_buf, BSDIFF_CONFIG_MAGIC, 8) != 0) {
        ty_adapt_free(buff);
        return -1;
    }

    /* Read lengths from patch header */
    bspatch.ctrllen = (uint32_t)offtin(read_buf + 8);
    bspatch.datalen = (uint32_t)offtin(read_buf + 16);
    bspatch.newsize = (uint32_t)offtin(read_buf + 24);
    ty_adapt_print("ctrllen:%d,  datalen:%d,  newsize:%d\r\n", bspatch.ctrllen, bspatch.datalen, bspatch.newsize);
    if (new_size != bspatch.newsize) {
        ty_adapt_print("new file size error\r\n");
    }
    
    bspatch.ctrl_num = bspatch.ctrllen / CTRL_BLOCK_LEN;
    ty_adapt_print("ctrl_num = %d\r\n", bspatch.ctrl_num);
    ty_adapt_print("ty_adapter_get_free_heap_size:%d\r\n", ty_adapter_get_free_heap_size());
    
    /* Apply patch */
    bspatch.oldpos = 0;
    bspatch.newpos = 0;
    
    int32_t read_len = 0;
    int32_t block_pos = 0;

    /* CTRL BLOCK READ */
    for (int32_t i = 0; i < bspatch.ctrl_num; i++) {
        
        if (patch1->read(patch1, read_buf, CTRL_BLOCK_LEN, arg) != CTRL_BLOCK_LEN) {
            ty_adapt_print("patch1 CTRL_BLOCK_LEN read fail!\r\n");
        }
        bspatch.ctrl.diff = (int32_t)offtin(read_buf);
        bspatch.ctrl.extra = (int32_t)offtin(read_buf + 8);
        bspatch.ctrl.oldnext = (int32_t)offtin(read_buf + 16);
        if (bspatch.ctrl.extra > 0) {
            ty_adapt_print("patch1, bspatch.ctrl.extra:%d, i:%d\r\n", bspatch.ctrl.extra, i);
            read_len = 0;
            int32_t extra_pos = 0;
            while (extra_pos < bspatch.ctrl.extra) {
                read_len = bspatch.ctrl.extra - extra_pos > READ_MAX ? READ_MAX : bspatch.ctrl.extra - extra_pos;
                int patch_read = patch1->read(patch1, read_buf, read_len, arg);
                if (patch_read != read_len) {
                    ty_adapt_print("patch1 Read extra string fail!, read_len:%d, patch_read:%d\r\n",read_len, patch_read);
                }
                extra_pos += read_len;
            }
        }
    }
    
    uint8_t forward_flag = !mode;
    uint32_t lcnt = new_size;
    uint32_t wrtotal = 0;
    uint32_t wrcnt = 0 ;
    uint32_t wrunit = 0;
    uint32_t maxcnt = new_size + step;
    uint32_t new_size_align = new_size % flash_cfg->blocksz;
    if(new_size < old_size) {
        maxcnt = old_size + step + new_size_align;
        if(maxcnt%flash_cfg->blocksz){
            maxcnt += flash_cfg->blocksz - (maxcnt % flash_cfg->blocksz);
        }
    }

    if(forward_flag) 
        wrunit = (lcnt > buf_size) ? buf_size : (new_size % buf_size);
    else {
        wrunit = (lcnt % buf_size) ? (new_size % buf_size) : buf_size;
    }
    
    uint32_t tmp_ctrlid = mag_info->ctrlid;

    for (int i = 0 ; i < bspatch.ctrl_num; i++) {
        //head
        if (patch->read(patch, read_buf, CTRL_BLOCK_LEN, arg) != CTRL_BLOCK_LEN) {
            ty_adapt_print("CTRL_BLOCK_LEN read fail!\r\n");
        }
        bspatch.ctrl.diff = (int32_t)offtin(read_buf);
        bspatch.ctrl.extra = (int32_t)offtin(read_buf + 8);
        bspatch.ctrl.oldnext = (int32_t)offtin(read_buf + 16);
        
        if(wrunit-wrcnt < bspatch.ctrl.diff + bspatch.ctrl.extra) {
            ty_adapt_print("[%d] wrunit: %d, wrcnt:%d  diff:%d, extra:%d, lcnt:%d\r\n",\
                i,wrunit,wrcnt,bspatch.ctrl.diff,bspatch.ctrl.extra, lcnt);
            ty_adapt_free(buff);
            return -1;
        }

        /* Read diff string */
        uint8_t old_buf[READ_MAX+4];
        read_len = 0;
        block_pos = 0;
        
        while (block_pos < bspatch.ctrl.diff) {
            read_len = bspatch.ctrl.diff - block_pos > READ_MAX ? READ_MAX : bspatch.ctrl.diff - block_pos;
           
            int patch_read = patch1->read(patch1, read_buf, read_len, arg);
            if (patch_read != read_len) {
                ty_adapt_print("Read diff string fail!, read_len:%d, patch_read:%d\r\n",read_len, patch_read);
            }

            if(i >= tmp_ctrlid) {
                /* Add old data to diff string */
                int ret = ty_adapt_flash_read(old_bin_addr + bspatch.oldpos + block_pos, old_buf, read_len, arg);
                if(ret) {
                    ty_adapt_free(buff);
                    return ret;
                }
                
                // if((i == 35) && (flagg1)) {
                //     flagg1 = 0;
                //     ty_adapt_print("*** addr : %x   oldpos %x   block_pos %x\r\n",old_bin_addr, bspatch.oldpos, block_pos);
                //     ty_adapt_print("***[%d-%d]  %02x %02x %02x %02x %02x %02x \r\n",wrcnt, read_len, old_buf[0],old_buf[1],old_buf[2],old_buf[3],old_buf[4],old_buf[5]);
                //     ty_adapt_print("***[%d-%d]  %02x %02x %02x %02x %02x %02x \r\n",wrcnt, read_len,read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5]);
                // }

                for (int32_t k = 0; k < read_len; k++) {
                    read_buf[k] += old_buf[k];
                }
                memcpy(buff + wrcnt, read_buf, read_len);
            }

            wrcnt += read_len;
            block_pos += read_len;
        }
        
        bspatch.newpos += bspatch.ctrl.diff;
        bspatch.oldpos += bspatch.ctrl.diff;

        
        if (bspatch.ctrl.extra > 0) {
            ty_adapt_print("patch0, bspatch.ctrl[i].extra:%d, i:%d\r\n", bspatch.ctrl.extra, i);
            read_len = 0;
            int32_t extra_pos = 0;
            //bspatch.newpos_extra = bspatch.newpos;
            while (extra_pos < bspatch.ctrl.extra) {
                read_len = bspatch.ctrl.extra - extra_pos > READ_MAX ? READ_MAX : bspatch.ctrl.extra - extra_pos;
                int patch_read = patch->read(patch, read_buf, read_len, arg);
                if (patch_read != read_len) {
                    ty_adapt_print("Read extra string fail!, read_len:%d, patch_read:%d\r\n",read_len, patch_read);
                }
                #if ___DEBUG
                if((i == 34) && (flagg)) {
                     flagg = 0;
                     ty_adapt_print("$$$$[%d-%d]   %02x %02x %02x %02x %02x %02x \r\n",wrcnt,read_len,read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5]);
                }
                #endif

                if(i >= tmp_ctrlid) {
                    memcpy(buff + wrcnt, read_buf, read_len);
                }

                wrcnt += read_len;
                extra_pos += read_len;
            }
        }

        /* pass extra pos */
        bspatch.newpos += bspatch.ctrl.extra;
        bspatch.oldpos += bspatch.ctrl.oldnext;
        if(wrcnt > wrunit) {
            ty_adapt_print("error...wrcnt:%d > wrunit:%d i:%d,error...\r\n", wrcnt, wrunit, i);
        }

        int ret = 0;
        uint32_t addr = 0;
        uint8_t flag = 0;
        if(wrunit == wrcnt) {
            if(forward_flag)
                addr = flh_info->frmaddr + wrtotal;
            else {
                addr = flh_info->frmaddr + maxcnt - wrtotal - wrcnt;
                if(addr%flash_cfg->blocksz) {
                    addr = addr - (addr%flash_cfg->blocksz);
                }
            }
                
            flag = 0;
            if(i + 1 == bspatch.ctrl_num) {
                flag = 1;
            }
            
            if(mag_info->state == BSDIFF_RESTORE1 && i + 1 == tmp_ctrlid) {
                ret = _bkup_2_frmware(mag, addr, buff, wrcnt, flag, arg);
            }else if(mag_info->state == BSDIFF_RESTORE0 && i >= tmp_ctrlid) {
                ret = _buff_2_bkup_2_frmware(mag, i+1, addr, buff, wrcnt, flag, arg);
            }

            // #if ___DEBUG
            ty_adapt_print("******************** rest.....state:%d, ctrlid:%d, wruint:%d........\r\n",mag_info->state, i,wrunit);
            // #endif

            if(ret) {
                ty_adapt_free(buff);
                ty_adapt_print("_bkup_2_frmware err...\r\n");
                return ret;
            }

            wrtotal += wrcnt;
            lcnt -= wrcnt;
            
            if(forward_flag)
                wrunit = (lcnt > buf_size) ? buf_size : (new_size % buf_size);
            else
                wrunit = (lcnt % buf_size) ? (new_size % buf_size) : buf_size;
                
            wrcnt = 0;
            bspatch.oldpos = 0;
            #if 0
            static uint32_t  tstcnt = 0;

            if(tstcnt++ ==  10) {
                ty_adapt_print("******************** rest.....ctrlid:%d........\r\n",i);
                return -1;
            }
            #endif
        }
    }

    ty_adapt_free(buff);
    ty_adapt_print("ty_adapter_get_free_heap_size:%d\r\n", ty_adapter_get_free_heap_size());
    return 0;
}




