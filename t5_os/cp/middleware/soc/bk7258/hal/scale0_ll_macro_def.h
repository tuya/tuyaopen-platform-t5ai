// Copyright 2020-2021 Beken 
// 
// Licensed under the Apache License, Version 2.0 (the "License");           
// you may not use this file except in compliance with the License.            
// You may obtain a copy of the License at                                     
//                                                                             
//     http://www.apache.org/licenses/LICENSE-2.0                              
//                                                                             
// Unless required by applicable law or agreed to in writing, software         
// distributed under the License is distributed on an "AS IS" BASIS,         
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    
// See the License for the specific language governing permissions and         
// limitations under the License.                                              

/***********************************************************************************************************************************
* This file is generated from BK7236_ADDR_Mapping_s.xlsm automatically                                
* Modify it manually is not recommended                                       
* CHIP ID:BK7236,GENARATE TIME:2023-08-08 17:57:22                                                 
************************************************************************************************************************************/

#pragma once                 
                            
#ifdef __cplusplus          
extern "C" {              
#endif                      

#include <soc/soc.h>          

#define SCALE0_LL_REG_BASE      (SOC_SCALE0_REG_BASE) //REG_BASE:0x480D0000

/* REG_0x00 */
#define SCALE0_0X00_ADDR  (SCALE0_LL_REG_BASE  + 0x0*4) //REG ADDR :0x480d0000
#define SCALE0_0X00_DEVID_POS (0) 
#define SCALE0_0X00_DEVID_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x00_value(void)
{
    return REG_READ(SCALE0_0X00_ADDR);
}

/* REG_0x00:0x00->DEVID: 0x00[31:0],Device ID ,0x48323634,R*/
static inline uint32_t scale0_ll_get_0x00_devid(void)
{
    return REG_READ(SCALE0_0X00_ADDR);
}

/* REG_0x01 */
#define SCALE0_0X01_ADDR  (SCALE0_LL_REG_BASE  + 0x1*4) //REG ADDR :0x480d0004
#define SCALE0_0X01_VERID_POS (0) 
#define SCALE0_0X01_VERID_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x01_value(void)
{
    return REG_READ(SCALE0_0X01_ADDR);
}

/* REG_0x01:0x01->VERID:0x01[31:0],Version ID ,0x00010001,R*/
static inline uint32_t scale0_ll_get_0x01_verid(void)
{
    return REG_READ(SCALE0_0X01_ADDR);
}

/* REG_0x02 */
#define SCALE0_0X02_ADDR  (SCALE0_LL_REG_BASE  + 0x2*4) //REG ADDR :0x480d0008
#define SCALE0_0X02_SOFT_RESET_POS (0) 
#define SCALE0_0X02_SOFT_RESET_MASK (0x1) 

#define SCALE0_0X02_CLKG_BYPASS_POS (1) 
#define SCALE0_0X02_CLKG_BYPASS_MASK (0x1) 

#define SCALE0_0X02_RESERVED0_POS (2) 
#define SCALE0_0X02_RESERVED0_MASK (0x3FFFFFFF) 

static inline uint32_t scale0_ll_get_0x02_value(void)
{
    return REG_READ(SCALE0_0X02_ADDR);
}

static inline void scale0_ll_set_0x02_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X02_ADDR,value);
}

/* REG_0x02:0x02->soft_reset: 0x02[0],software reset,1,RW*/
static inline uint32_t scale0_ll_get_0x02_soft_reset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X02_ADDR);
    reg_value = ((reg_value >> SCALE0_0X02_SOFT_RESET_POS) & SCALE0_0X02_SOFT_RESET_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x02_soft_reset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X02_ADDR);
    reg_value &= ~(SCALE0_0X02_SOFT_RESET_MASK << SCALE0_0X02_SOFT_RESET_POS);
    reg_value |= ((value & SCALE0_0X02_SOFT_RESET_MASK) << SCALE0_0X02_SOFT_RESET_POS);
    REG_WRITE(SCALE0_0X02_ADDR,reg_value);
}

/* REG_0x02:0x02->clkg_bypass: 0x02[1],pclk gating bypass,0,RW*/
static inline uint32_t scale0_ll_get_0x02_clkg_bypass(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X02_ADDR);
    reg_value = ((reg_value >> SCALE0_0X02_CLKG_BYPASS_POS) & SCALE0_0X02_CLKG_BYPASS_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x02_clkg_bypass(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X02_ADDR);
    reg_value &= ~(SCALE0_0X02_CLKG_BYPASS_MASK << SCALE0_0X02_CLKG_BYPASS_POS);
    reg_value |= ((value & SCALE0_0X02_CLKG_BYPASS_MASK) << SCALE0_0X02_CLKG_BYPASS_POS);
    REG_WRITE(SCALE0_0X02_ADDR,reg_value);
}

/* REG_0x03 */
#define SCALE0_0X03_ADDR  (SCALE0_LL_REG_BASE  + 0x3*4) //REG ADDR :0x480d000c
#define SCALE0_0X03_DEVSTATUS_POS (0) 
#define SCALE0_0X03_DEVSTATUS_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x03_value(void)
{
    return REG_READ(SCALE0_0X03_ADDR);
}

/* REG_0x03:0x03->Devstatus: 0x03[31:0],Device status,0,R*/
static inline uint32_t scale0_ll_get_0x03_devstatus(void)
{
    return REG_READ(SCALE0_0X03_ADDR);
}

/* REG_0x04 */
#define SCALE0_0X04_ADDR  (SCALE0_LL_REG_BASE  + 0x4*4) //REG ADDR :0x480d0010
#define SCALE0_0X04_FIRST_ADDR_POS (0) 
#define SCALE0_0X04_FIRST_ADDR_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x04_value(void)
{
    return REG_READ(SCALE0_0X04_ADDR);
}

static inline void scale0_ll_set_0x04_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X04_ADDR,value);
}

/* REG_0x04:0x04->first_addr:0x04[31:0],Valid in YUV422 mode when data originates from JPGD.The addr need loop back to first addr when reaches ping-pong gap.,0,RW*/
static inline uint32_t scale0_ll_get_0x04_first_addr(void)
{
    return REG_READ(SCALE0_0X04_ADDR);
}

static inline void scale0_ll_set_0x04_first_addr(uint32_t value)
{
    REG_WRITE(SCALE0_0X04_ADDR,value);
}

/* REG_0x05 */
#define SCALE0_0X05_ADDR  (SCALE0_LL_REG_BASE  + 0x5*4) //REG ADDR :0x480d0014
#define SCALE0_0X05_LINE_INDEX_POS (0) 
#define SCALE0_0X05_LINE_INDEX_MASK (0x1) 

#define SCALE0_0X05_RESERVED0_POS (1) 
#define SCALE0_0X05_RESERVED0_MASK (0x7FFFFFFF) 

static inline uint32_t scale0_ll_get_0x05_value(void)
{
    return REG_READ(SCALE0_0X05_ADDR);
}

static inline void scale0_ll_set_0x05_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X05_ADDR,value);
}

/* REG_0x05:0x05->line_index:0x05[0],Valid in YUV422 mode when data originates from JPGD.; 1'b0: 8line ping pong buffer; 1'b1: 16line ping pong buffer,0,RW*/
static inline uint32_t scale0_ll_get_0x05_line_index(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X05_ADDR);
    reg_value = ((reg_value >> SCALE0_0X05_LINE_INDEX_POS) & SCALE0_0X05_LINE_INDEX_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x05_line_index(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X05_ADDR);
    reg_value &= ~(SCALE0_0X05_LINE_INDEX_MASK << SCALE0_0X05_LINE_INDEX_POS);
    reg_value |= ((value & SCALE0_0X05_LINE_INDEX_MASK) << SCALE0_0X05_LINE_INDEX_POS);
    REG_WRITE(SCALE0_0X05_ADDR,reg_value);
}

/* REG_0x06 */
#define SCALE0_0X06_ADDR  (SCALE0_LL_REG_BASE  + 0x6*4) //REG ADDR :0x480d0018
#define SCALE0_0X06_COL_COEF_POS (0) 
#define SCALE0_0X06_COL_COEF_MASK (0xFF) 

#define SCALE0_0X06_RESERVED0_POS (8) 
#define SCALE0_0X06_RESERVED0_MASK (0xFFFFFF) 

static inline uint32_t scale0_ll_get_0x06_value(void)
{
    return REG_READ(SCALE0_0X06_ADDR);
}

static inline void scale0_ll_set_0x06_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X06_ADDR,value);
}

/* REG_0x06:0x06->col_coef:0x06[7:0],sw updates this reg in row_done int ,0,RW*/
static inline uint32_t scale0_ll_get_0x06_col_coef(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X06_ADDR);
    reg_value = ((reg_value >> SCALE0_0X06_COL_COEF_POS) & SCALE0_0X06_COL_COEF_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x06_col_coef(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X06_ADDR);
    reg_value &= ~(SCALE0_0X06_COL_COEF_MASK << SCALE0_0X06_COL_COEF_POS);
    reg_value |= ((value & SCALE0_0X06_COL_COEF_MASK) << SCALE0_0X06_COL_COEF_POS);
    REG_WRITE(SCALE0_0X06_ADDR,reg_value);
}

/* REG_0x07 */
#define SCALE0_0X07_ADDR  (SCALE0_LL_REG_BASE  + 0x7*4) //REG ADDR :0x480d001c
#define SCALE0_0X07_MODULE_START_POS (0) 
#define SCALE0_0X07_MODULE_START_MASK (0x1) 

#define SCALE0_0X07_RESERVED0_POS (1) 
#define SCALE0_0X07_RESERVED0_MASK (0x7FFFFFFF) 

static inline uint32_t scale0_ll_get_0x07_value(void)
{
    return REG_READ(SCALE0_0X07_ADDR);
}

static inline void scale0_ll_set_0x07_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X07_ADDR,value);
}

/* REG_0x07:0x07->module_start:0x07[0],SW write 1'b1 to start next row operation in each row_done int,0,RW*/
static inline uint32_t scale0_ll_get_0x07_module_start(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X07_ADDR);
    reg_value = ((reg_value >> SCALE0_0X07_MODULE_START_POS) & SCALE0_0X07_MODULE_START_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x07_module_start(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X07_ADDR);
    reg_value &= ~(SCALE0_0X07_MODULE_START_MASK << SCALE0_0X07_MODULE_START_POS);
    reg_value |= ((value & SCALE0_0X07_MODULE_START_MASK) << SCALE0_0X07_MODULE_START_POS);
    REG_WRITE(SCALE0_0X07_ADDR,reg_value);
}

/* REG_0x08 */
#define SCALE0_0X08_ADDR  (SCALE0_LL_REG_BASE  + 0x8*4) //REG ADDR :0x480d0020
#define SCALE0_0X08_COM_CFG_POS (0) 
#define SCALE0_0X08_COM_CFG_MASK (0x7) 

#define SCALE0_0X08_RESERVED0_POS (3) 
#define SCALE0_0X08_RESERVED0_MASK (0x1FFFFFFF) 

static inline uint32_t scale0_ll_get_0x08_value(void)
{
    return REG_READ(SCALE0_0X08_ADDR);
}

static inline void scale0_ll_set_0x08_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X08_ADDR,value);
}

/* REG_0x08:0x08->com_cfg:0x08[2:0],interrupt enable; [0]: row done int en; [1]: RSVD; [2]: RSVD; ,0,RW*/
static inline uint32_t scale0_ll_get_0x08_com_cfg(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X08_ADDR);
    reg_value = ((reg_value >> SCALE0_0X08_COM_CFG_POS) & SCALE0_0X08_COM_CFG_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x08_com_cfg(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X08_ADDR);
    reg_value &= ~(SCALE0_0X08_COM_CFG_MASK << SCALE0_0X08_COM_CFG_POS);
    reg_value |= ((value & SCALE0_0X08_COM_CFG_MASK) << SCALE0_0X08_COM_CFG_POS);
    REG_WRITE(SCALE0_0X08_ADDR,reg_value);
}

/* REG_0x09 */
#define SCALE0_0X09_ADDR  (SCALE0_LL_REG_BASE  + 0x9*4) //REG ADDR :0x480d0024
#define SCALE0_0X09_FORMAT_POS (0) 
#define SCALE0_0X09_FORMAT_MASK (0x3) 

#define SCALE0_0X09_RESERVED0_POS (2) 
#define SCALE0_0X09_RESERVED0_MASK (0x3FFFFFFF) 

static inline uint32_t scale0_ll_get_0x09_value(void)
{
    return REG_READ(SCALE0_0X09_ADDR);
}

static inline void scale0_ll_set_0x09_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X09_ADDR,value);
}

/* REG_0x09:0x09->format:0x09[1:0],2'b00: RGB565; 2'b01: YUV422; 2'b10: RGB888,0,RW*/
static inline uint32_t scale0_ll_get_0x09_format(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X09_ADDR);
    reg_value = ((reg_value >> SCALE0_0X09_FORMAT_POS) & SCALE0_0X09_FORMAT_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x09_format(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X09_ADDR);
    reg_value &= ~(SCALE0_0X09_FORMAT_MASK << SCALE0_0X09_FORMAT_POS);
    reg_value |= ((value & SCALE0_0X09_FORMAT_MASK) << SCALE0_0X09_FORMAT_POS);
    REG_WRITE(SCALE0_0X09_ADDR,reg_value);
}

/* REG_0x0A */
#define SCALE0_0X0A_ADDR  (SCALE0_LL_REG_BASE  + 0xA*4) //REG ADDR :0x480d0028
#define SCALE0_0X0A_BASE_ADDR_POS (0) 
#define SCALE0_0X0A_BASE_ADDR_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x0a_value(void)
{
    return REG_READ(SCALE0_0X0A_ADDR);
}

static inline void scale0_ll_set_0x0a_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0A_ADDR,value);
}

/* REG_0x0a:0x0a->base_addr:0x0a[31:0],SW updates this reg;  to inform hw where;  to fetch source data,0,RW*/
static inline uint32_t scale0_ll_get_0x0a_base_addr(void)
{
    return REG_READ(SCALE0_0X0A_ADDR);
}

static inline void scale0_ll_set_0x0a_base_addr(uint32_t value)
{
    REG_WRITE(SCALE0_0X0A_ADDR,value);
}

/* REG_0x0B */
#define SCALE0_0X0B_ADDR  (SCALE0_LL_REG_BASE  + 0xB*4) //REG ADDR :0x480d002c
#define SCALE0_0X0B_PIC_WIDTH_POS (0) 
#define SCALE0_0X0B_PIC_WIDTH_MASK (0x7FF) 

#define SCALE0_0X0B_RESERVED0_POS (11) 
#define SCALE0_0X0B_RESERVED0_MASK (0x1FFFFF) 

static inline uint32_t scale0_ll_get_0x0b_value(void)
{
    return REG_READ(SCALE0_0X0B_ADDR);
}

static inline void scale0_ll_set_0x0b_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0B_ADDR,value);
}

/* REG_0x0b:0x0b->pic_width:0x0b[10:0],source picture width,0,RW*/
static inline uint32_t scale0_ll_get_0x0b_pic_width(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0B_ADDR);
    reg_value = ((reg_value >> SCALE0_0X0B_PIC_WIDTH_POS) & SCALE0_0X0B_PIC_WIDTH_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x0b_pic_width(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0B_ADDR);
    reg_value &= ~(SCALE0_0X0B_PIC_WIDTH_MASK << SCALE0_0X0B_PIC_WIDTH_POS);
    reg_value |= ((value & SCALE0_0X0B_PIC_WIDTH_MASK) << SCALE0_0X0B_PIC_WIDTH_POS);
    REG_WRITE(SCALE0_0X0B_ADDR,reg_value);
}

/* REG_0x0C */
#define SCALE0_0X0C_ADDR  (SCALE0_LL_REG_BASE  + 0xC*4) //REG ADDR :0x480d0030
#define SCALE0_0X0C_DEST_ADDR_POS (0) 
#define SCALE0_0X0C_DEST_ADDR_MASK (0xFFFFFFFF) 

static inline uint32_t scale0_ll_get_0x0c_value(void)
{
    return REG_READ(SCALE0_0X0C_ADDR);
}

static inline void scale0_ll_set_0x0c_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0C_ADDR,value);
}

/* REG_0x0c:0x0c->dest_addr:0x0c[31:0],sw updates this reg to inform hw where to store resized data,0,RW*/
static inline uint32_t scale0_ll_get_0x0c_dest_addr(void)
{
    return REG_READ(SCALE0_0X0C_ADDR);
}

static inline void scale0_ll_set_0x0c_dest_addr(uint32_t value)
{
    REG_WRITE(SCALE0_0X0C_ADDR,value);
}

/* REG_0x0D */
#define SCALE0_0X0D_ADDR  (SCALE0_LL_REG_BASE  + 0xD*4) //REG ADDR :0x480d0034
#define SCALE0_0X0D_OUT_PIC_WIDTH_POS (0) 
#define SCALE0_0X0D_OUT_PIC_WIDTH_MASK (0x7FF) 

#define SCALE0_0X0D_RESERVED0_POS (11) 
#define SCALE0_0X0D_RESERVED0_MASK (0x1FFFFF) 

static inline uint32_t scale0_ll_get_0x0d_value(void)
{
    return REG_READ(SCALE0_0X0D_ADDR);
}

static inline void scale0_ll_set_0x0d_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0D_ADDR,value);
}

/* REG_0x0d:0x0d->out_pic_width:0x0d[10:0],Resized pic width,0,RW*/
static inline uint32_t scale0_ll_get_0x0d_out_pic_width(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0D_ADDR);
    reg_value = ((reg_value >> SCALE0_0X0D_OUT_PIC_WIDTH_POS) & SCALE0_0X0D_OUT_PIC_WIDTH_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x0d_out_pic_width(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0D_ADDR);
    reg_value &= ~(SCALE0_0X0D_OUT_PIC_WIDTH_MASK << SCALE0_0X0D_OUT_PIC_WIDTH_POS);
    reg_value |= ((value & SCALE0_0X0D_OUT_PIC_WIDTH_MASK) << SCALE0_0X0D_OUT_PIC_WIDTH_POS);
    REG_WRITE(SCALE0_0X0D_ADDR,reg_value);
}

/* REG_0x0E */
#define SCALE0_0X0E_ADDR  (SCALE0_LL_REG_BASE  + 0xE*4) //REG ADDR :0x480d0038
#define SCALE0_0X0E_HLOC_INDEX_POS (0) 
#define SCALE0_0X0E_HLOC_INDEX_MASK (0x7FF) 

#define SCALE0_0X0E_RESERVED0_POS (11) 
#define SCALE0_0X0E_RESERVED0_MASK (0x1FFFFF) 

static inline uint32_t scale0_ll_get_0x0e_value(void)
{
    return REG_READ(SCALE0_0X0E_ADDR);
}

static inline void scale0_ll_set_0x0e_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0E_ADDR,value);
}

/* REG_0x0e:0x0e->hloc_index:0x0e[10:0],sw updates this reg to inform hw which pic row to fetch data,0,RW*/
static inline uint32_t scale0_ll_get_0x0e_hloc_index(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0E_ADDR);
    reg_value = ((reg_value >> SCALE0_0X0E_HLOC_INDEX_POS) & SCALE0_0X0E_HLOC_INDEX_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x0e_hloc_index(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0E_ADDR);
    reg_value &= ~(SCALE0_0X0E_HLOC_INDEX_MASK << SCALE0_0X0E_HLOC_INDEX_POS);
    reg_value |= ((value & SCALE0_0X0E_HLOC_INDEX_MASK) << SCALE0_0X0E_HLOC_INDEX_POS);
    REG_WRITE(SCALE0_0X0E_ADDR,reg_value);
}

/* REG_0x0F */
#define SCALE0_0X0F_ADDR  (SCALE0_LL_REG_BASE  + 0xF*4) //REG ADDR :0x480d003c
#define SCALE0_0X0F_CLR_INT_STAT_POS (0) 
#define SCALE0_0X0F_CLR_INT_STAT_MASK (0x1) 

#define SCALE0_0X0F_RESERVED0_POS (3) 
#define SCALE0_0X0F_RESERVED0_MASK (0x1FFFFFFF) 

static inline uint32_t scale0_ll_get_0x0f_value(void)
{
    return REG_READ(SCALE0_0X0F_ADDR);
}

static inline void scale0_ll_set_0x0f_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X0F_ADDR,value);
}

/* REG_0x0f:0x0f->clr_int_stat:0x0f[2:0],[0]: row done int clear; [1]: RSVD; [2]: RSVD,0,RW*/
static inline uint32_t scale0_ll_get_0x0f_clr_int_stat(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0F_ADDR);
    reg_value = ((reg_value >> SCALE0_0X0F_CLR_INT_STAT_POS) & SCALE0_0X0F_CLR_INT_STAT_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x0f_clr_int_stat(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X0F_ADDR);
    reg_value &= ~(SCALE0_0X0F_CLR_INT_STAT_MASK << SCALE0_0X0F_CLR_INT_STAT_POS);
    reg_value |= ((value & SCALE0_0X0F_CLR_INT_STAT_MASK) << SCALE0_0X0F_CLR_INT_STAT_POS);
    REG_WRITE(SCALE0_0X0F_ADDR,reg_value);
}

/* REG_0x10 */
#define SCALE0_0X10_ADDR  (SCALE0_LL_REG_BASE  + 0x10*4) //REG ADDR :0x480d0040
#define SCALE0_0X10_R_WRITE_THRESHOLD_POS (0) 
#define SCALE0_0X10_R_WRITE_THRESHOLD_MASK (0xFFF) 

#define SCALE0_0X10_RESERVED0_POS (12) 
#define SCALE0_0X10_RESERVED0_MASK (0xFFFFF) 

static inline uint32_t scale0_ll_get_0x10_value(void)
{
    return REG_READ(SCALE0_0X10_ADDR);
}

static inline void scale0_ll_set_0x10_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X10_ADDR,value);
}

/* REG_0x10:0x10->r_write_threshold:0x10[11:0],4: BURST 4; 8: BURST 8; 16: BURST 16; 32: BURST 32; 64: BURST 64; Others: BURST 32,0,RW*/
static inline uint32_t scale0_ll_get_0x10_r_write_threshold(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X10_ADDR);
    reg_value = ((reg_value >> SCALE0_0X10_R_WRITE_THRESHOLD_POS) & SCALE0_0X10_R_WRITE_THRESHOLD_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x10_r_write_threshold(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X10_ADDR);
    reg_value &= ~(SCALE0_0X10_R_WRITE_THRESHOLD_MASK << SCALE0_0X10_R_WRITE_THRESHOLD_POS);
    reg_value |= ((value & SCALE0_0X10_R_WRITE_THRESHOLD_MASK) << SCALE0_0X10_R_WRITE_THRESHOLD_POS);
    REG_WRITE(SCALE0_0X10_ADDR,reg_value);
}

/* REG_0x11 */
#define SCALE0_0X11_ADDR  (SCALE0_LL_REG_BASE  + 0x11*4) //REG ADDR :0x480d0044
#define SCALE0_0X11_INT_STAT_POS (0) 
#define SCALE0_0X11_INT_STAT_MASK (0x7) 

#define SCALE0_0X11_RESERVED0_POS (3) 
#define SCALE0_0X11_RESERVED0_MASK (0x1FFFFFFF) 

static inline uint32_t scale0_ll_get_0x11_value(void)
{
    return REG_READ(SCALE0_0X11_ADDR);
}

/* REG_0x11:0x11->int_stat:0x11[2:0],bit[0]: row_done interrupt; bit[2:1]: RSVD,0,R*/
static inline uint32_t scale0_ll_get_0x11_int_stat(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X11_ADDR);
    reg_value = ((reg_value >> SCALE0_0X11_INT_STAT_POS)&SCALE0_0X11_INT_STAT_MASK);
    return reg_value;
}

/* REG_0x20 */
#define SCALE0_0X20_ADDR  (SCALE0_LL_REG_BASE  + 0x20*4) //REG ADDR :0x480d0080
#define SCALE0_0X20_ROW_COEF_LOC_PARA0_POS (0) 
#define SCALE0_0X20_ROW_COEF_LOC_PARA0_MASK (0xFFFF) 

#define SCALE0_0X20_RESERVED0_POS (16) 
#define SCALE0_0X20_RESERVED0_MASK (0xFFFF) 

static inline uint32_t scale0_ll_get_0x20_value(void)
{
    return REG_READ(SCALE0_0X20_ADDR);
}

static inline void scale0_ll_set_0x20_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X20_ADDR,value);
}

/* REG_0x20:0x20->row_coef_loc_para0:0x20[15:0],bit[3:0]: row coefficient; bit[15:4]: location,0,RW*/
static inline uint32_t scale0_ll_get_0x20_row_coef_loc_para0(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X20_ADDR);
    reg_value = ((reg_value >> SCALE0_0X20_ROW_COEF_LOC_PARA0_POS) & SCALE0_0X20_ROW_COEF_LOC_PARA0_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x20_row_coef_loc_para0(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X20_ADDR);
    reg_value &= ~(SCALE0_0X20_ROW_COEF_LOC_PARA0_MASK << SCALE0_0X20_ROW_COEF_LOC_PARA0_POS);
    reg_value |= ((value & SCALE0_0X20_ROW_COEF_LOC_PARA0_MASK) << SCALE0_0X20_ROW_COEF_LOC_PARA0_POS);
    REG_WRITE(SCALE0_0X20_ADDR,reg_value);
}

/* REG_0x520 */
#define SCALE0_0X520_ADDR  (SCALE0_LL_REG_BASE  + 0x520*4) //REG ADDR :0x480d1480
#define SCALE0_0X520_ROW_COEF_LOC_PARA1279_POS (0) 
#define SCALE0_0X520_ROW_COEF_LOC_PARA1279_MASK (0xFFFF) 

#define SCALE0_0X520_RESERVED0_POS (16) 
#define SCALE0_0X520_RESERVED0_MASK (0xFFFF) 

static inline uint32_t scale0_ll_get_0x520_value(void)
{
    return REG_READ(SCALE0_0X520_ADDR);
}

static inline void scale0_ll_set_0x520_value(uint32_t value)
{
    REG_WRITE(SCALE0_0X520_ADDR,value);
}

/* REG_0x520:0x520->row_coef_loc_para1279:0x520[15:0],bit[3:0]: row coefficient; bit[15:4]: location,0,RW*/
static inline uint32_t scale0_ll_get_0x520_row_coef_loc_para1279(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X520_ADDR);
    reg_value = ((reg_value >> SCALE0_0X520_ROW_COEF_LOC_PARA1279_POS) & SCALE0_0X520_ROW_COEF_LOC_PARA1279_MASK);
    return reg_value;
}

static inline void scale0_ll_set_0x520_row_coef_loc_para1279(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SCALE0_0X520_ADDR);
    reg_value &= ~(SCALE0_0X520_ROW_COEF_LOC_PARA1279_MASK << SCALE0_0X520_ROW_COEF_LOC_PARA1279_POS);
    reg_value |= ((value & SCALE0_0X520_ROW_COEF_LOC_PARA1279_MASK) << SCALE0_0X520_ROW_COEF_LOC_PARA1279_POS);
    REG_WRITE(SCALE0_0X520_ADDR,reg_value);
}

#ifdef __cplusplus 
}                  
#endif             
