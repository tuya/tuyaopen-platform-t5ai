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
                            
#ifdef __cplusplus          
extern "C" {              
#endif                      

typedef volatile struct {

    /* REG_0x00 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  DeviceID                       :32;  //31:0,ASCII码 "mail",0x6D61696C,R
        }; 
        uint32_t v; 
    }reg_0x0; 

    /* REG_0x01 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  VersionID                      :32;  //31:0,设备版本号 2.0,0x20000,R
        }; 
        uint32_t v; 
    }reg_0x1; 

    /* REG_0x02 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  softrstn                       : 1;  //0,softrstn,0x0,R/W
           volatile uint32_t  hclk_bps                       : 1;  //1,Hclk门控Bypass,0x0,R/W
           volatile uint32_t  chn_pro_disable                : 1;  //2,0:chn受保护，cpu不能写不属于自己的chan，受硬件保护.  1: chn不受保护，任意master可以写任意chn,软件保证chn的使用合法性。,0x0,R/W
           volatile uint32_t  reserved3                      :29;  
        }; 
        uint32_t v; 
    }reg_0x2; 

    /* REG_0x03 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  int_status_chn0                : 1;  //0,ch0 中断状态 0：无中断。  1： 有中断,0x0,R
           volatile uint32_t  int_status_chn1                : 1;  //1,ch1 中断状态 0：无中断。  1： 有中断,0x0,R
           volatile uint32_t  int_status_chn2                : 1;  //2,ch2 中断状态 0：无中断。  1： 有中断,0x0,R
           volatile uint32_t  reserved3                      :29;  
        }; 
        uint32_t v; 
    }reg_0x3; 

    /* REG_0x04 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x4_reserved; 

    /* REG_0x05 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x5_reserved; 

    /* REG_0x06 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x6_reserved; 

    /* REG_0x07 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x7_reserved; 

    /* REG_0x08 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x8_reserved; 

    /* REG_0x09 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x9_reserved; 

    /* REG_0x0a */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xa_reserved; 

    /* REG_0x0b */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xb_reserved; 

    /* REG_0x0c */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xc_reserved; 

    /* REG_0x0d */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xd_reserved; 

    /* REG_0x0e */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xe_reserved; 

    /* REG_0x0f */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0xf_reserved; 

    /* REG_0x10 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_start                     : 6;  //5:0,chn0的fifo起始地址,0x0,R/W
           volatile uint32_t  reserved6                      : 2;  
           volatile uint32_t  int_en                         : 1;  //8,1: chn0 int enable 0: chn0 int disable,0x1,R/W
           volatile uint32_t  int_wrerr_en                   : 1;  //9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W
           volatile uint32_t  int_rderr_en                   : 1;  //10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W
           volatile uint32_t  int_wrfull_en                  : 1;  //11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W
           volatile uint32_t  reserved12                     : 4;  
           volatile uint32_t  wrerr_int_sta                  : 1;  //16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C
           volatile uint32_t  rderr_int_sta                  : 1;  //17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C
           volatile uint32_t  wrfull_int_sta                 : 1;  //18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C
           volatile uint32_t  fifo_noempt                    : 1;  //19,0:fifo 空  1： fifo非空,0x0,R
           volatile uint32_t  reserved20                     :12;  
        }; 
        uint32_t v; 
    }reg_0x10; 

    /* REG_0x11 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  chn_enable                     : 1;  //0,通道使能信号，必须先写fifo起始地址后写通道使能,0x0,R/W
           volatile uint32_t  fifo_length                    : 6;  //6:1,chn0的fifo的深度,0x0,R/W
           volatile uint32_t  reserved7                      :25;  
        }; 
        uint32_t v; 
    }reg_0x11; 

    /* REG_0x12 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata0                    :32;  //31:0,向tid发送的数据tdata0,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x12; 

    /* REG_0x13 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata1                    :32;  //31:0,向tid发送的数据tdata1,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x13; 

    /* REG_0x14 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tid                       : 4;  //3:0,接收端tid,兼开始发送功能,0x0,R/W
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x14; 

    /* REG_0x15 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_sid                       : 4;  //3:0,发送端sid,兼开始接收功能,0x0,R
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x15; 

    /* REG_0x16 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata0                    :32;  //31:0,读数据rdata0,0x0,R
        }; 
        uint32_t v; 
    }reg_0x16; 

    /* REG_0x17 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata1                    :32;  //31:0,读数据rdata1,0x0,R
        }; 
        uint32_t v; 
    }reg_0x17; 

    /* REG_0x18 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_full                      : 1;  //0,fifo满标志,0x0,R
           volatile uint32_t  fifo_empt                      : 1;  //1,fifo空标志,0x0,R
           volatile uint32_t  fifo_count                     : 6;  //7:2,当前fifo内数据个数,0x0,R
           volatile uint32_t  reserved8                      :24;  
        }; 
        uint32_t v; 
    }reg_0x18; 

    /* REG_0x19 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x19_reserved; 

    /* REG_0x1a */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1a_reserved; 

    /* REG_0x1b */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1b_reserved; 

    /* REG_0x1c */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1c_reserved; 

    /* REG_0x1d */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1d_reserved; 

    /* REG_0x1e */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1e_reserved; 

    /* REG_0x1f */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x1f_reserved; 

    /* REG_0x20 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_start                     : 6;  //5:0,chn1的fifo起始地址,0x0,R/W
           volatile uint32_t  reserved6                      : 2;  
           volatile uint32_t  int_en                         : 1;  //8,1: chn1 int enable 0: chn1 int disable,0x1,R/W
           volatile uint32_t  int_wrerr_en                   : 1;  //9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W
           volatile uint32_t  int_rderr_en                   : 1;  //10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W
           volatile uint32_t  int_wrfull_en                  : 1;  //11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W
           volatile uint32_t  reserved12                     : 4;  
           volatile uint32_t  wrerr_int_sta                  : 1;  //16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C
           volatile uint32_t  rderr_int_sta                  : 1;  //17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C
           volatile uint32_t  wrfull_int_sta                 : 1;  //18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C
           volatile uint32_t  fifo_noempt                    : 1;  //19,0:fifo 空  1： fifo非空,0x0,R
           volatile uint32_t  reserved20                     :12;  
        }; 
        uint32_t v; 
    }reg_0x20; 

    /* REG_0x21 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  chn_enable                     : 1;  //0,使能信号,0x0,R/W
           volatile uint32_t  fifo_length                    : 6;  //6:1,chn1的fifo的深度,0x0,R/W
           volatile uint32_t  reserved7                      :25;  
        }; 
        uint32_t v; 
    }reg_0x21; 

    /* REG_0x22 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata0                    :32;  //31:0,向tid发送的数据tdata0,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x22; 

    /* REG_0x23 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata1                    :32;  //31:0,向tid发送的数据tdata1,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x23; 

    /* REG_0x24 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tid                       : 4;  //3:0,接收端tid,兼开始发送功能,0x0,R/W
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x24; 

    /* REG_0x25 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_sid                       : 4;  //3:0,发送端sid,兼开始接收功能,0x0,R
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x25; 

    /* REG_0x26 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata0                    :32;  //31:0,读数据rdata0,0x0,R
        }; 
        uint32_t v; 
    }reg_0x26; 

    /* REG_0x27 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata1                    :32;  //31:0,读数据rdata1,0x0,R
        }; 
        uint32_t v; 
    }reg_0x27; 

    /* REG_0x28 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_full                      : 1;  //0,fifo满标志,0x0,R
           volatile uint32_t  fifo_empt                      : 1;  //1,fifo空标志,0x0,R
           volatile uint32_t  fifo_count                     : 6;  //7:2,当前fifo内数据个数,0x0,R
           volatile uint32_t  reserved8                      :24;  
        }; 
        uint32_t v; 
    }reg_0x28; 

    /* REG_0x29 */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x29_reserved; 

    /* REG_0x2a */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2a_reserved; 

    /* REG_0x2b */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2b_reserved; 

    /* REG_0x2c */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2c_reserved; 

    /* REG_0x2d */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2d_reserved; 

    /* REG_0x2e */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2e_reserved; 

    /* REG_0x2f */
    union 
    { 
        struct 
        { 
           volatile uint32_t reserved:32;  
        }; 
        uint32_t v; 
    }reg_0x2f_reserved; 

    /* REG_0x30 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_start                     : 6;  //5:0,chn2的fifo起始地址,0x0,R/W
           volatile uint32_t  reserved6                      : 2;  
           volatile uint32_t  int_en                         : 1;  //8,1: chn2 int enable 0: chn2 int disable,0x1,R/W
           volatile uint32_t  int_wrerr_en                   : 1;  //9,1:写异常中断使能    0：写异常中断不使能,0x0,R/W
           volatile uint32_t  int_rderr_en                   : 1;  //10,1:读异常中断使能    0：读异常中断不使能,0x0,R/W
           volatile uint32_t  int_wrfull_en                  : 1;  //11,1:写满异常中断使能  0：写满异常中断不使能,0x1,R/W
           volatile uint32_t  reserved12                     : 4;  
           volatile uint32_t  wrerr_int_sta                  : 1;  //16,写异常中断状态   ，写1清。被非法通道写入mail_tid置1或tid越界置1,0x0,R/W1C
           volatile uint32_t  rderr_int_sta                  : 1;  //17,读异常中断状态   ，写1清。被非法通道读取mail_sid置1,0x0,R/W1C
           volatile uint32_t  wrfull_int_sta                 : 1;  //18,写满异常中断状态 ，写1清。向满fifo写入置1,0x0,R/W1C
           volatile uint32_t  fifo_noempt                    : 1;  //19,0:fifo 空  1： fifo非空,0x0,R
           volatile uint32_t  reserved20                     :12;  
        }; 
        uint32_t v; 
    }reg_0x30; 

    /* REG_0x31 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  chn_enable                     : 1;  //0,使能信号,0x0,R/W
           volatile uint32_t  fifo_length                    : 6;  //6:1,chn2的fifo的深度,0x0,R/W
           volatile uint32_t  reserved7                      :25;  
        }; 
        uint32_t v; 
    }reg_0x31; 

    /* REG_0x32 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata0                    :32;  //31:0,向tid发送的数据tdata0,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x32; 

    /* REG_0x33 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tdata1                    :32;  //31:0,向tid发送的数据tdata1,0x0,R/W
        }; 
        uint32_t v; 
    }reg_0x33; 

    /* REG_0x34 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_tid                       : 4;  //3:0,接收端tid,兼开始发送功能,0x0,R/W
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x34; 

    /* REG_0x35 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_sid                       : 4;  //3:0,发送端sid,兼开始接收功能,0x0,R
           volatile uint32_t  reserved4                      :28;  
        }; 
        uint32_t v; 
    }reg_0x35; 

    /* REG_0x36 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata0                    :32;  //31:0,读数据rdata0,0x0,R
        }; 
        uint32_t v; 
    }reg_0x36; 

    /* REG_0x37 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  mail_rdata1                    :32;  //31:0,读数据rdata1,0x0,R
        }; 
        uint32_t v; 
    }reg_0x37; 

    /* REG_0x38 */
    union 
    { 
        struct 
        { 
           volatile uint32_t  fifo_full                      : 1;  //0,fifo满标志,0x0,R
           volatile uint32_t  fifo_empt                      : 1;  //1,fifo空标志,0x0,R
           volatile uint32_t  fifo_count                     : 6;  //7:2,当前fifo内数据个数,0x0,R
           volatile uint32_t  reserved8                      :24;  
        }; 
        uint32_t v; 
    }reg_0x38; 

} mbox0_hw_t;

#ifdef __cplusplus 
}                  
#endif             
