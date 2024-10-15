// Copyright 2022-2023 Beken 
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

#pragma once

#include <stdint.h>
#include "mailbox_hw.h"
#include "mbox0_struct.h" 

#ifdef __cplusplus
extern "C" {
#endif

#define MBOX0_FIFO_SIZE   	   8
#define MBOX0_LL_REG_BASE     (MAILBOX_BASE)

static inline uint32_t mbox0_ll_get_reg_base(void)
{
	return MBOX0_LL_REG_BASE;
}

static inline uint32_t mbox0_ll_get_fifo_size(void)
{
	return MBOX0_FIFO_SIZE;
}


/* REG_0x00 */

static inline uint32_t mbox0_ll_get_reg_0x0_value(mbox0_hw_t *hw)
{
    return hw->reg_0x0.v;
}

/* REG_0x00:reg_0x0->DeviceID:31:0,ASCII码 "mail",0x6D61696C,R*/
static inline uint32_t mbox0_ll_get_reg_0x0_DeviceID(mbox0_hw_t *hw)
{
    return hw->reg_0x0.v;
}

/* REG_0x01 */

static inline uint32_t mbox0_ll_get_reg_0x1_value(mbox0_hw_t *hw)
{
    return hw->reg_0x1.v;
}

/* REG_0x01:reg_0x1->VersionID:31:0,设备版本号 2.0,0x20000,R*/
static inline uint32_t mbox0_ll_get_reg_0x1_VersionID(mbox0_hw_t *hw)
{
    return hw->reg_0x1.v;
}

/* REG_0x02 */

static inline uint32_t mbox0_ll_get_reg_0x2_value(mbox0_hw_t *hw)
{
    return hw->reg_0x2.v;
}

static inline void mbox0_ll_set_reg_0x2_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x2.v = value;
}

/* REG_0x02:reg_0x2->softrstn:0,softrstn,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x2_softrstn(mbox0_hw_t *hw)
{
    return hw->reg_0x2.softrstn;
}

/* REG_0x02:reg_0x2->softrstn:0,softrstn,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x2_softrstn(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x2.softrstn = value;
}

/* REG_0x02:reg_0x2->hclk_bps:1,Hclk门控Bypass,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x2_hclk_bps(mbox0_hw_t *hw)
{
    return hw->reg_0x2.hclk_bps;
}

/* REG_0x02:reg_0x2->hclk_bps:1,Hclk门控Bypass,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x2_hclk_bps(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x2.hclk_bps = value;
}

/* REG_0x02:reg_0x2->chn_pro_disable:2,0:chn受保护，cpu不能写不属于自己的chan，受硬件保护.  1: chn不受保护，任意master可以写任意chn,软件保证chn的使用合法性。,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x2_chn_pro_disable(mbox0_hw_t *hw)
{
    return hw->reg_0x2.chn_pro_disable;
}

/* REG_0x02:reg_0x2->chn_pro_disable:2,0:chn受保护，cpu不能写不属于自己的chan，受硬件保护.  1: chn不受保护，任意master可以写任意chn,软件保证chn的使用合法性。,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x2_chn_pro_disable(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x2.chn_pro_disable = value;
}

/* REG_0x03 */

static inline uint32_t mbox0_ll_get_reg_0x3_value(mbox0_hw_t *hw)
{
    return hw->reg_0x3.v;
}

/* REG_0x03:reg_0x3->int_status_chn0:0,ch0 中断状态 0：无中断。  1： 有中断,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x3_int_status_chn0(mbox0_hw_t *hw)
{
    return hw->reg_0x3.int_status_chn0;
}

/* REG_0x03:reg_0x3->int_status_chn1:1,ch1 中断状态 0：无中断。  1： 有中断,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x3_int_status_chn1(mbox0_hw_t *hw)
{
    return hw->reg_0x3.int_status_chn1;
}

/* REG_0x03:reg_0x3->int_status_chn2:2,ch2 中断状态 0：无中断。  1： 有中断,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x3_int_status_chn2(mbox0_hw_t *hw)
{
    return hw->reg_0x3.int_status_chn2;
}

/* REG_0x10 */

static inline uint32_t mbox0_ll_get_reg_0x10_value(mbox0_hw_t *hw)
{
    return hw->reg_0x10.v;
}

static inline void mbox0_ll_set_reg_0x10_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.v = value;
}

/* REG_0x10:reg_0x10->fifo_start:5:0,chn0的fifo起始地址,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x10_fifo_start(mbox0_hw_t *hw)
{
    return hw->reg_0x10.fifo_start;
}

/* REG_0x10:reg_0x10->fifo_start:5:0,chn0的fifo起始地址,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x10_fifo_start(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.fifo_start = value;
}

/* REG_0x10:reg_0x10->int_en:8,1: chn0 int enable 0: chn0 int disable,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x10_int_en(mbox0_hw_t *hw)
{
    return hw->reg_0x10.int_en;
}

/* REG_0x10:reg_0x10->int_en:8,1: chn0 int enable 0: chn0 int disable,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x10_int_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.int_en = value;
}

/* REG_0x10:reg_0x10->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x10_int_wrerr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x10.int_wrerr_en;
}

/* REG_0x10:reg_0x10->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x10_int_wrerr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.int_wrerr_en = value;
}

/* REG_0x10:reg_0x10->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x10_int_rderr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x10.int_rderr_en;
}

/* REG_0x10:reg_0x10->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x10_int_rderr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.int_rderr_en = value;
}

/* REG_0x10:reg_0x10->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x10_int_wrfull_en(mbox0_hw_t *hw)
{
    return hw->reg_0x10.int_wrfull_en;
}

/* REG_0x10:reg_0x10->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x10_int_wrfull_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.int_wrfull_en = value;
}

/* REG_0x10:reg_0x10->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x10_wrerr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x10.wrerr_int_sta;
}

/* REG_0x10:reg_0x10->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x10_wrerr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.wrerr_int_sta = value;
}

/* REG_0x10:reg_0x10->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x10_rderr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x10.rderr_int_sta;
}

/* REG_0x10:reg_0x10->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x10_rderr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.rderr_int_sta = value;
}

/* REG_0x10:reg_0x10->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x10_wrfull_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x10.wrfull_int_sta;
}

/* REG_0x10:reg_0x10->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x10_wrfull_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x10.wrfull_int_sta = value;
}

/* REG_0x10:reg_0x10->fifo_noempt:19,0:fifo 空  1： fifo非空,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x10_fifo_noempt(mbox0_hw_t *hw)
{
    return hw->reg_0x10.fifo_noempt;
}

/* REG_0x11 */

static inline uint32_t mbox0_ll_get_reg_0x11_value(mbox0_hw_t *hw)
{
    return hw->reg_0x11.v;
}

static inline void mbox0_ll_set_reg_0x11_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x11.v = value;
}

/* REG_0x11:reg_0x11->chn_enable:0,通道使能信号，必须先写fifo起始地址后写通道使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x11_chn_enable(mbox0_hw_t *hw)
{
    return hw->reg_0x11.chn_enable;
}

/* REG_0x11:reg_0x11->chn_enable:0,通道使能信号，必须先写fifo起始地址后写通道使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x11_chn_enable(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x11.chn_enable = value;
}

/* REG_0x11:reg_0x11->fifo_length:6:1,chn0的fifo的深度,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x11_fifo_length(mbox0_hw_t *hw)
{
    return hw->reg_0x11.fifo_length;
}

/* REG_0x11:reg_0x11->fifo_length:6:1,chn0的fifo的深度,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x11_fifo_length(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x11.fifo_length = value;
}

/* REG_0x12 */

static inline uint32_t mbox0_ll_get_reg_0x12_value(mbox0_hw_t *hw)
{
    return hw->reg_0x12.v;
}

static inline void mbox0_ll_set_reg_0x12_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x12.v = value;
}

/* REG_0x12:reg_0x12->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x12_mail_tdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x12.v;
}

/* REG_0x12:reg_0x12->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x12_mail_tdata0(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x12.v = value;
}

/* REG_0x13 */

static inline uint32_t mbox0_ll_get_reg_0x13_value(mbox0_hw_t *hw)
{
    return hw->reg_0x13.v;
}

static inline void mbox0_ll_set_reg_0x13_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x13.v = value;
}

/* REG_0x13:reg_0x13->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x13_mail_tdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x13.v;
}

/* REG_0x13:reg_0x13->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x13_mail_tdata1(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x13.v = value;
}

/* REG_0x14 */

static inline uint32_t mbox0_ll_get_reg_0x14_value(mbox0_hw_t *hw)
{
    return hw->reg_0x14.v;
}

static inline void mbox0_ll_set_reg_0x14_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x14.v = value;
}

/* REG_0x14:reg_0x14->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x14_mail_tid(mbox0_hw_t *hw)
{
    return hw->reg_0x14.mail_tid;
}

/* REG_0x14:reg_0x14->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x14_mail_tid(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x14.mail_tid = value;
}

/* REG_0x15 */

static inline uint32_t mbox0_ll_get_reg_0x15_value(mbox0_hw_t *hw)
{
    return hw->reg_0x15.v;
}

/* REG_0x15:reg_0x15->mail_sid:3:0,发送端sid,兼开始接收功能,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x15_mail_sid(mbox0_hw_t *hw)
{
    return hw->reg_0x15.mail_sid;
}

/* REG_0x16 */

static inline uint32_t mbox0_ll_get_reg_0x16_value(mbox0_hw_t *hw)
{
    return hw->reg_0x16.v;
}

/* REG_0x16:reg_0x16->mail_rdata0:31:0,读数据rdata0,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x16_mail_rdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x16.v;
}

/* REG_0x17 */

static inline uint32_t mbox0_ll_get_reg_0x17_value(mbox0_hw_t *hw)
{
    return hw->reg_0x17.v;
}

/* REG_0x17:reg_0x17->mail_rdata1:31:0,读数据rdata1,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x17_mail_rdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x17.v;
}

/* REG_0x18 */

static inline uint32_t mbox0_ll_get_reg_0x18_value(mbox0_hw_t *hw)
{
    return hw->reg_0x18.v;
}

/* REG_0x18:reg_0x18->fifo_full:0,fifo满标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x18_fifo_full(mbox0_hw_t *hw)
{
    return hw->reg_0x18.fifo_full;
}

/* REG_0x18:reg_0x18->fifo_empt:1,fifo空标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x18_fifo_empt(mbox0_hw_t *hw)
{
    return hw->reg_0x18.fifo_empt;
}

/* REG_0x18:reg_0x18->fifo_count:7:2,当前fifo内数据个数,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x18_fifo_count(mbox0_hw_t *hw)
{
    return hw->reg_0x18.fifo_count;
}

/* REG_0x20 */

static inline uint32_t mbox0_ll_get_reg_0x20_value(mbox0_hw_t *hw)
{
    return hw->reg_0x20.v;
}

static inline void mbox0_ll_set_reg_0x20_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.v = value;
}

/* REG_0x20:reg_0x20->fifo_start:5:0,chn1的fifo起始地址,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x20_fifo_start(mbox0_hw_t *hw)
{
    return hw->reg_0x20.fifo_start;
}

/* REG_0x20:reg_0x20->fifo_start:5:0,chn1的fifo起始地址,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x20_fifo_start(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.fifo_start = value;
}

/* REG_0x20:reg_0x20->int_en:8,1: chn1 int enable 0: chn1 int disable,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x20_int_en(mbox0_hw_t *hw)
{
    return hw->reg_0x20.int_en;
}

/* REG_0x20:reg_0x20->int_en:8,1: chn1 int enable 0: chn1 int disable,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x20_int_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.int_en = value;
}

/* REG_0x20:reg_0x20->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x20_int_wrerr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x20.int_wrerr_en;
}

/* REG_0x20:reg_0x20->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x20_int_wrerr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.int_wrerr_en = value;
}

/* REG_0x20:reg_0x20->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x20_int_rderr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x20.int_rderr_en;
}

/* REG_0x20:reg_0x20->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x20_int_rderr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.int_rderr_en = value;
}

/* REG_0x20:reg_0x20->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x20_int_wrfull_en(mbox0_hw_t *hw)
{
    return hw->reg_0x20.int_wrfull_en;
}

/* REG_0x20:reg_0x20->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x20_int_wrfull_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.int_wrfull_en = value;
}

/* REG_0x20:reg_0x20->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x20_wrerr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x20.wrerr_int_sta;
}

/* REG_0x20:reg_0x20->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x20_wrerr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.wrerr_int_sta = value;
}

/* REG_0x20:reg_0x20->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x20_rderr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x20.rderr_int_sta;
}

/* REG_0x20:reg_0x20->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x20_rderr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.rderr_int_sta = value;
}

/* REG_0x20:reg_0x20->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x20_wrfull_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x20.wrfull_int_sta;
}

/* REG_0x20:reg_0x20->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x20_wrfull_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x20.wrfull_int_sta = value;
}

/* REG_0x20:reg_0x20->fifo_noempt:19,0:fifo 空  1： fifo非空,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x20_fifo_noempt(mbox0_hw_t *hw)
{
    return hw->reg_0x20.fifo_noempt;
}

/* REG_0x21 */

static inline uint32_t mbox0_ll_get_reg_0x21_value(mbox0_hw_t *hw)
{
    return hw->reg_0x21.v;
}

static inline void mbox0_ll_set_reg_0x21_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x21.v = value;
}

/* REG_0x21:reg_0x21->chn_enable:0,使能信号,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x21_chn_enable(mbox0_hw_t *hw)
{
    return hw->reg_0x21.chn_enable;
}

/* REG_0x21:reg_0x21->chn_enable:0,使能信号,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x21_chn_enable(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x21.chn_enable = value;
}

/* REG_0x21:reg_0x21->fifo_length:6:1,chn1的fifo的深度,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x21_fifo_length(mbox0_hw_t *hw)
{
    return hw->reg_0x21.fifo_length;
}

/* REG_0x21:reg_0x21->fifo_length:6:1,chn1的fifo的深度,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x21_fifo_length(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x21.fifo_length = value;
}

/* REG_0x22 */

static inline uint32_t mbox0_ll_get_reg_0x22_value(mbox0_hw_t *hw)
{
    return hw->reg_0x22.v;
}

static inline void mbox0_ll_set_reg_0x22_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x22.v = value;
}

/* REG_0x22:reg_0x22->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x22_mail_tdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x22.v;
}

/* REG_0x22:reg_0x22->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x22_mail_tdata0(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x22.v = value;
}

/* REG_0x23 */

static inline uint32_t mbox0_ll_get_reg_0x23_value(mbox0_hw_t *hw)
{
    return hw->reg_0x23.v;
}

static inline void mbox0_ll_set_reg_0x23_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x23.v = value;
}

/* REG_0x23:reg_0x23->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x23_mail_tdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x23.v;
}

/* REG_0x23:reg_0x23->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x23_mail_tdata1(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x23.v = value;
}

/* REG_0x24 */

static inline uint32_t mbox0_ll_get_reg_0x24_value(mbox0_hw_t *hw)
{
    return hw->reg_0x24.v;
}

static inline void mbox0_ll_set_reg_0x24_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x24.v = value;
}

/* REG_0x24:reg_0x24->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x24_mail_tid(mbox0_hw_t *hw)
{
    return hw->reg_0x24.mail_tid;
}

/* REG_0x24:reg_0x24->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x24_mail_tid(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x24.mail_tid = value;
}

/* REG_0x25 */

static inline uint32_t mbox0_ll_get_reg_0x25_value(mbox0_hw_t *hw)
{
    return hw->reg_0x25.v;
}

/* REG_0x25:reg_0x25->mail_sid:3:0,发送端sid,兼开始接收功能,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x25_mail_sid(mbox0_hw_t *hw)
{
    return hw->reg_0x25.mail_sid;
}

/* REG_0x26 */

static inline uint32_t mbox0_ll_get_reg_0x26_value(mbox0_hw_t *hw)
{
    return hw->reg_0x26.v;
}

/* REG_0x26:reg_0x26->mail_rdata0:31:0,读数据rdata0,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x26_mail_rdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x26.v;
}

/* REG_0x27 */

static inline uint32_t mbox0_ll_get_reg_0x27_value(mbox0_hw_t *hw)
{
    return hw->reg_0x27.v;
}

/* REG_0x27:reg_0x27->mail_rdata1:31:0,读数据rdata1,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x27_mail_rdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x27.v;
}

/* REG_0x28 */

static inline uint32_t mbox0_ll_get_reg_0x28_value(mbox0_hw_t *hw)
{
    return hw->reg_0x28.v;
}

/* REG_0x28:reg_0x28->fifo_full:0,fifo满标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x28_fifo_full(mbox0_hw_t *hw)
{
    return hw->reg_0x28.fifo_full;
}

/* REG_0x28:reg_0x28->fifo_empt:1,fifo空标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x28_fifo_empt(mbox0_hw_t *hw)
{
    return hw->reg_0x28.fifo_empt;
}

/* REG_0x28:reg_0x28->fifo_count:7:2,当前fifo内数据个数,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x28_fifo_count(mbox0_hw_t *hw)
{
    return hw->reg_0x28.fifo_count;
}

/* REG_0x30 */

static inline uint32_t mbox0_ll_get_reg_0x30_value(mbox0_hw_t *hw)
{
    return hw->reg_0x30.v;
}

static inline void mbox0_ll_set_reg_0x30_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.v = value;
}

/* REG_0x30:reg_0x30->fifo_start:5:0,chn2的fifo起始地址,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x30_fifo_start(mbox0_hw_t *hw)
{
    return hw->reg_0x30.fifo_start;
}

/* REG_0x30:reg_0x30->fifo_start:5:0,chn2的fifo起始地址,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x30_fifo_start(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.fifo_start = value;
}

/* REG_0x30:reg_0x30->int_en:8,1: chn2 int enable 0: chn2 int disable,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x30_int_en(mbox0_hw_t *hw)
{
    return hw->reg_0x30.int_en;
}

/* REG_0x30:reg_0x30->int_en:8,1: chn2 int enable 0: chn2 int disable,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x30_int_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.int_en = value;
}

/* REG_0x30:reg_0x30->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x30_int_wrerr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x30.int_wrerr_en;
}

/* REG_0x30:reg_0x30->int_wrerr_en:9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x30_int_wrerr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.int_wrerr_en = value;
}

/* REG_0x30:reg_0x30->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x30_int_rderr_en(mbox0_hw_t *hw)
{
    return hw->reg_0x30.int_rderr_en;
}

/* REG_0x30:reg_0x30->int_rderr_en:10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x30_int_rderr_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.int_rderr_en = value;
}

/* REG_0x30:reg_0x30->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x30_int_wrfull_en(mbox0_hw_t *hw)
{
    return hw->reg_0x30.int_wrfull_en;
}

/* REG_0x30:reg_0x30->int_wrfull_en:11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W*/
static inline void mbox0_ll_set_reg_0x30_int_wrfull_en(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.int_wrfull_en = value;
}

/* REG_0x30:reg_0x30->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x30_wrerr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x30.wrerr_int_sta;
}

/* REG_0x30:reg_0x30->wrerr_int_sta:16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x30_wrerr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.wrerr_int_sta = value;
}

/* REG_0x30:reg_0x30->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x30_rderr_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x30.rderr_int_sta;
}

/* REG_0x30:reg_0x30->rderr_int_sta:17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x30_rderr_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.rderr_int_sta = value;
}

/* REG_0x30:reg_0x30->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline uint32_t mbox0_ll_get_reg_0x30_wrfull_int_sta(mbox0_hw_t *hw)
{
    return hw->reg_0x30.wrfull_int_sta;
}

/* REG_0x30:reg_0x30->wrfull_int_sta:18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C*/
static inline void mbox0_ll_set_reg_0x30_wrfull_int_sta(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x30.wrfull_int_sta = value;
}

/* REG_0x30:reg_0x30->fifo_noempt:19,0:fifo 空  1： fifo非空,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x30_fifo_noempt(mbox0_hw_t *hw)
{
    return hw->reg_0x30.fifo_noempt;
}

/* REG_0x31 */

static inline uint32_t mbox0_ll_get_reg_0x31_value(mbox0_hw_t *hw)
{
    return hw->reg_0x31.v;
}

static inline void mbox0_ll_set_reg_0x31_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x31.v = value;
}

/* REG_0x31:reg_0x31->chn_enable:0,使能信号,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x31_chn_enable(mbox0_hw_t *hw)
{
    return hw->reg_0x31.chn_enable;
}

/* REG_0x31:reg_0x31->chn_enable:0,使能信号,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x31_chn_enable(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x31.chn_enable = value;
}

/* REG_0x31:reg_0x31->fifo_length:6:1,chn2的fifo的深度,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x31_fifo_length(mbox0_hw_t *hw)
{
    return hw->reg_0x31.fifo_length;
}

/* REG_0x31:reg_0x31->fifo_length:6:1,chn2的fifo的深度,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x31_fifo_length(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x31.fifo_length = value;
}

/* REG_0x32 */

static inline uint32_t mbox0_ll_get_reg_0x32_value(mbox0_hw_t *hw)
{
    return hw->reg_0x32.v;
}

static inline void mbox0_ll_set_reg_0x32_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x32.v = value;
}

/* REG_0x32:reg_0x32->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x32_mail_tdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x32.v;
}

/* REG_0x32:reg_0x32->mail_tdata0:31:0,向tid发送的数据tdata0,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x32_mail_tdata0(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x32.v = value;
}

/* REG_0x33 */

static inline uint32_t mbox0_ll_get_reg_0x33_value(mbox0_hw_t *hw)
{
    return hw->reg_0x33.v;
}

static inline void mbox0_ll_set_reg_0x33_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x33.v = value;
}

/* REG_0x33:reg_0x33->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x33_mail_tdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x33.v;
}

/* REG_0x33:reg_0x33->mail_tdata1:31:0,向tid发送的数据tdata1,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x33_mail_tdata1(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x33.v = value;
}

/* REG_0x34 */

static inline uint32_t mbox0_ll_get_reg_0x34_value(mbox0_hw_t *hw)
{
    return hw->reg_0x34.v;
}

static inline void mbox0_ll_set_reg_0x34_value(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x34.v = value;
}

/* REG_0x34:reg_0x34->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline uint32_t mbox0_ll_get_reg_0x34_mail_tid(mbox0_hw_t *hw)
{
    return hw->reg_0x34.mail_tid;
}

/* REG_0x34:reg_0x34->mail_tid:3:0,接收端tid,兼开始发送功能,0x0,R/W*/
static inline void mbox0_ll_set_reg_0x34_mail_tid(mbox0_hw_t *hw, uint32_t value)
{
    hw->reg_0x34.mail_tid = value;
}

/* REG_0x35 */

static inline uint32_t mbox0_ll_get_reg_0x35_value(mbox0_hw_t *hw)
{
    return hw->reg_0x35.v;
}

/* REG_0x35:reg_0x35->mail_sid:3:0,发送端sid,兼开始接收功能,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x35_mail_sid(mbox0_hw_t *hw)
{
    return hw->reg_0x35.mail_sid;
}

/* REG_0x36 */

static inline uint32_t mbox0_ll_get_reg_0x36_value(mbox0_hw_t *hw)
{
    return hw->reg_0x36.v;
}

/* REG_0x36:reg_0x36->mail_rdata0:31:0,读数据rdata0,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x36_mail_rdata0(mbox0_hw_t *hw)
{
    return hw->reg_0x36.v;
}

/* REG_0x37 */

static inline uint32_t mbox0_ll_get_reg_0x37_value(mbox0_hw_t *hw)
{
    return hw->reg_0x37.v;
}

/* REG_0x37:reg_0x37->mail_rdata1:31:0,读数据rdata1,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x37_mail_rdata1(mbox0_hw_t *hw)
{
    return hw->reg_0x37.v;
}

/* REG_0x38 */

static inline uint32_t mbox0_ll_get_reg_0x38_value(mbox0_hw_t *hw)
{
    return hw->reg_0x38.v;
}

/* REG_0x38:reg_0x38->fifo_full:0,fifo满标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x38_fifo_full(mbox0_hw_t *hw)
{
    return hw->reg_0x38.fifo_full;
}

/* REG_0x38:reg_0x38->fifo_empt:1,fifo空标志,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x38_fifo_empt(mbox0_hw_t *hw)
{
    return hw->reg_0x38.fifo_empt;
}

/* REG_0x38:reg_0x38->fifo_count:7:2,当前fifo内数据个数,0x0,R*/
static inline uint32_t mbox0_ll_get_reg_0x38_fifo_count(mbox0_hw_t *hw)
{
    return hw->reg_0x38.fifo_count;
}


#ifdef __cplusplus 
}                  
#endif             
