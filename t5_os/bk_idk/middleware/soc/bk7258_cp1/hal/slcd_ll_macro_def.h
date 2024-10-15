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


#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "soc/soc.h"

#define SLCD_LL_REG_BASE      (SOC_SLCD_REG_BASE) //REG ADDR :0x458e0000

/* REG_0x00 */
#define SLCD_DEVICE_ID_ADDR  (SLCD_LL_REG_BASE  + 0x0*4) //REG ADDR :0x458e0000
#define SLCD_DEVICE_ID_DEVICE_ID_POS (0)
#define SLCD_DEVICE_ID_DEVICE_ID_MASK (0xFFFFFFFF)

static inline uint32_t slcd_ll_get_device_id_value(void)
{
    return REG_READ(SLCD_DEVICE_ID_ADDR);
}

/* REG_0x00:device_id->device_id:0x0[31:0],Device ID:ASCII Code "slcd",0x534c4344,R*/
static inline uint32_t slcd_ll_get_device_id_device_id(void)
{
    return REG_READ(SLCD_DEVICE_ID_ADDR);
}

/* REG_0x01 */
#define SLCD_VERSION_ID_ADDR  (SLCD_LL_REG_BASE  + 0x1*4) //REG ADDR :0x458e0004
#define SLCD_VERSION_ID_VERSION_ID_POS (0)
#define SLCD_VERSION_ID_VERSION_ID_MASK (0xFFFFFFFF)

static inline uint32_t slcd_ll_get_version_id_value(void)
{
    return REG_READ(SLCD_VERSION_ID_ADDR);
}

/* REG_0x01:version_id->version_id:0x1[31:0],Version ID: 版本号 V1.1,0x00010001,R*/
static inline uint32_t slcd_ll_get_version_id_version_id(void)
{
    return REG_READ(SLCD_VERSION_ID_ADDR);
}

/* REG_0x02 */
#define SLCD_CLK_CONTROL_ADDR  (SLCD_LL_REG_BASE  + 0x2*4) //REG ADDR :0x458e0008
#define SLCD_CLK_CONTROL_SOFT_RESET_POS (0)
#define SLCD_CLK_CONTROL_SOFT_RESET_MASK (0x1)

#define SLCD_CLK_CONTROL_BYPASS_CKG_POS (1)
#define SLCD_CLK_CONTROL_BYPASS_CKG_MASK (0x1)

#define SLCD_CLK_CONTROL_RESERVED0_POS (2)
#define SLCD_CLK_CONTROL_RESERVED0_MASK (0x3FFFFFFF)

static inline uint32_t slcd_ll_get_clk_control_value(void)
{
    return REG_READ(SLCD_CLK_CONTROL_ADDR);
}

static inline void slcd_ll_set_clk_control_value(uint32_t value)
{
    REG_WRITE(SLCD_CLK_CONTROL_ADDR,value);
}

/* REG_0x02:clk_control->soft_reset:0x2[0],0x0,R/W*/
static inline uint32_t slcd_ll_get_clk_control_soft_reset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CLK_CONTROL_ADDR);
    reg_value = ((reg_value >> SLCD_CLK_CONTROL_SOFT_RESET_POS) & SLCD_CLK_CONTROL_SOFT_RESET_MASK);
    return reg_value;
}

static inline void slcd_ll_set_clk_control_soft_reset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CLK_CONTROL_ADDR);
    reg_value &= ~(SLCD_CLK_CONTROL_SOFT_RESET_MASK << SLCD_CLK_CONTROL_SOFT_RESET_POS);
    reg_value |= ((value & SLCD_CLK_CONTROL_SOFT_RESET_MASK) << SLCD_CLK_CONTROL_SOFT_RESET_POS);
    REG_WRITE(SLCD_CLK_CONTROL_ADDR,reg_value);
}

/* REG_0x02:clk_control->clk_gate:0x2[1],0x0,R/W*/
static inline uint32_t slcd_ll_get_clk_control_bypass_ckg(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CLK_CONTROL_ADDR);
    reg_value = ((reg_value >> SLCD_CLK_CONTROL_BYPASS_CKG_POS) & SLCD_CLK_CONTROL_BYPASS_CKG_MASK);
    return reg_value;
}

static inline void slcd_ll_set_clk_control_bypass_ckg(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CLK_CONTROL_ADDR);
    reg_value &= ~(SLCD_CLK_CONTROL_BYPASS_CKG_MASK << SLCD_CLK_CONTROL_BYPASS_CKG_POS);
    reg_value |= ((value & SLCD_CLK_CONTROL_BYPASS_CKG_MASK) << SLCD_CLK_CONTROL_BYPASS_CKG_POS);
    REG_WRITE(SLCD_CLK_CONTROL_ADDR,reg_value);
}

/* REG_0x03 */
#define SLCD_GLOBAL_STATUS_ADDR  (SLCD_LL_REG_BASE  + 0x3*4) //REG ADDR :0x458e000c
#define SLCD_GLOBAL_STATUS_GLOBAL_STATUS_POS (0)
#define SLCD_GLOBAL_STATUS_GLOBAL_STATUS_MASK (0xFFFFFFFF)

static inline uint32_t slcd_ll_get_global_status_value(void)
{
    return REG_READ(SLCD_GLOBAL_STATUS_ADDR);
}

/* REG_0x03:global_status->global_Status:0x3[31:0],0x0,R*/
static inline uint32_t slcd_ll_get_global_status_global_status(void)
{
    return REG_READ(SLCD_GLOBAL_STATUS_ADDR);
}

/* REG_0x04 */
#define SLCD_SEG00_03_ADDR  (SLCD_LL_REG_BASE  + 0x4*4) //REG ADDR :0x458e0010
#define SLCD_SEG00_03_SEG00_POS (0)
#define SLCD_SEG00_03_SEG00_MASK (0xFF)
#define SLCD_SEG00_03_SEG01_POS (8)
#define SLCD_SEG00_03_SEG01_MASK (0xFF)
#define SLCD_SEG00_03_SEG02_POS (16)
#define SLCD_SEG00_03_SEG02_MASK (0xFF)
#define SLCD_SEG00_03_SEG03_POS (24)
#define SLCD_SEG00_03_SEG03_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg00_03_value(void)
{
    return REG_READ(SLCD_SEG00_03_ADDR);
}

static inline void slcd_ll_set_seg00_03_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG00_03_ADDR,value);
}

/* REG_0x04:seg00_03->seg00:0x4[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg00_03_seg00(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value = ((reg_value >> SLCD_SEG00_03_SEG00_POS) & SLCD_SEG00_03_SEG00_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg00_03_seg00(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value &= ~(SLCD_SEG00_03_SEG00_MASK << SLCD_SEG00_03_SEG00_POS);
    reg_value |= ((value & SLCD_SEG00_03_SEG00_MASK) << SLCD_SEG00_03_SEG00_POS);
    REG_WRITE(SLCD_SEG00_03_ADDR,reg_value);
}

/* REG_0x04:seg00_03->seg01:0x4[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg00_03_seg01(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value = ((reg_value >> SLCD_SEG00_03_SEG01_POS) & SLCD_SEG00_03_SEG01_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg00_03_seg01(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value &= ~(SLCD_SEG00_03_SEG01_MASK << SLCD_SEG00_03_SEG01_POS);
    reg_value |= ((value & SLCD_SEG00_03_SEG01_MASK) << SLCD_SEG00_03_SEG01_POS);
    REG_WRITE(SLCD_SEG00_03_ADDR,reg_value);
}

/* REG_0x04:seg00_03->seg02:0x4[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg00_03_seg02(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value = ((reg_value >> SLCD_SEG00_03_SEG02_POS) & SLCD_SEG00_03_SEG02_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg00_03_seg02(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value &= ~(SLCD_SEG00_03_SEG02_MASK << SLCD_SEG00_03_SEG02_POS);
    reg_value |= ((value & SLCD_SEG00_03_SEG02_MASK) << SLCD_SEG00_03_SEG02_POS);
    REG_WRITE(SLCD_SEG00_03_ADDR,reg_value);
}

/* REG_0x04:seg00_03->seg03:0x4[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg00_03_seg03(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value = ((reg_value >> SLCD_SEG00_03_SEG03_POS) & SLCD_SEG00_03_SEG03_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg00_03_seg03(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG00_03_ADDR);
    reg_value &= ~(SLCD_SEG00_03_SEG03_MASK << SLCD_SEG00_03_SEG03_POS);
    reg_value |= ((value & SLCD_SEG00_03_SEG03_MASK) << SLCD_SEG00_03_SEG03_POS);
    REG_WRITE(SLCD_SEG00_03_ADDR,reg_value);
}

/* REG_0x05 */
#define SLCD_SEG04_07_ADDR  (SLCD_LL_REG_BASE  + 0x5*4) //REG ADDR :0x458e0014
#define SLCD_SEG04_07_SEG04_POS (0)
#define SLCD_SEG04_07_SEG04_MASK (0xFF)
#define SLCD_SEG04_07_SEG05_POS (8)
#define SLCD_SEG04_07_SEG05_MASK (0xFF)
#define SLCD_SEG04_07_SEG06_POS (16)
#define SLCD_SEG04_07_SEG06_MASK (0xFF)
#define SLCD_SEG04_07_SEG07_POS (24)
#define SLCD_SEG04_07_SEG07_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg04_07_value(void)
{
    return REG_READ(SLCD_SEG04_07_ADDR);
}

static inline void slcd_ll_set_seg04_07_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG04_07_ADDR,value);
}

/* REG_0x05:seg04_07->seg04:0x5[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg04_07_seg04(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value = ((reg_value >> SLCD_SEG04_07_SEG04_POS) & SLCD_SEG04_07_SEG04_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg04_07_seg04(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value &= ~(SLCD_SEG04_07_SEG04_MASK << SLCD_SEG04_07_SEG04_POS);
    reg_value |= ((value & SLCD_SEG04_07_SEG04_MASK) << SLCD_SEG04_07_SEG04_POS);
    REG_WRITE(SLCD_SEG04_07_ADDR,reg_value);
}

/* REG_0x05:seg04_07->seg05:0x5[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg04_07_seg05(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value = ((reg_value >> SLCD_SEG04_07_SEG05_POS) & SLCD_SEG04_07_SEG05_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg04_07_seg05(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value &= ~(SLCD_SEG04_07_SEG05_MASK << SLCD_SEG04_07_SEG05_POS);
    reg_value |= ((value & SLCD_SEG04_07_SEG05_MASK) << SLCD_SEG04_07_SEG05_POS);
    REG_WRITE(SLCD_SEG04_07_ADDR,reg_value);
}

/* REG_0x05:seg04_07->seg06:0x5[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg04_07_seg06(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value = ((reg_value >> SLCD_SEG04_07_SEG06_POS) & SLCD_SEG04_07_SEG06_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg04_07_seg06(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value &= ~(SLCD_SEG04_07_SEG06_MASK << SLCD_SEG04_07_SEG06_POS);
    reg_value |= ((value & SLCD_SEG04_07_SEG06_MASK) << SLCD_SEG04_07_SEG06_POS);
    REG_WRITE(SLCD_SEG04_07_ADDR,reg_value);
}

/* REG_0x05:seg04_07->seg07:0x5[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg04_07_seg07(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value = ((reg_value >> SLCD_SEG04_07_SEG07_POS) & SLCD_SEG04_07_SEG07_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg04_07_seg07(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG04_07_ADDR);
    reg_value &= ~(SLCD_SEG04_07_SEG07_MASK << SLCD_SEG04_07_SEG07_POS);
    reg_value |= ((value & SLCD_SEG04_07_SEG07_MASK) << SLCD_SEG04_07_SEG07_POS);
    REG_WRITE(SLCD_SEG04_07_ADDR,reg_value);
}

/* REG_0x06 */
#define SLCD_SEG08_11_ADDR  (SLCD_LL_REG_BASE  + 0x6*4) //REG ADDR :0x458e0018
#define SLCD_SEG08_11_SEG08_POS (0)
#define SLCD_SEG08_11_SEG08_MASK (0xFF)
#define SLCD_SEG08_11_SEG09_POS (8)
#define SLCD_SEG08_11_SEG09_MASK (0xFF)
#define SLCD_SEG08_11_SEG10_POS (16)
#define SLCD_SEG08_11_SEG10_MASK (0xFF)
#define SLCD_SEG08_11_SEG11_POS (24)
#define SLCD_SEG08_11_SEG11_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg08_11_value(void)
{
    return REG_READ(SLCD_SEG08_11_ADDR);
}

static inline void slcd_ll_set_seg08_11_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG08_11_ADDR,value);
}

/* REG_0x06:seg08_11->seg08:0x6[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg08_11_seg08(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value = ((reg_value >> SLCD_SEG08_11_SEG08_POS) & SLCD_SEG08_11_SEG08_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg08_11_seg08(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value &= ~(SLCD_SEG08_11_SEG08_MASK << SLCD_SEG08_11_SEG08_POS);
    reg_value |= ((value & SLCD_SEG08_11_SEG08_MASK) << SLCD_SEG08_11_SEG08_POS);
    REG_WRITE(SLCD_SEG08_11_ADDR,reg_value);
}

/* REG_0x06:seg08_11->seg09:0x6[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg08_11_seg09(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value = ((reg_value >> SLCD_SEG08_11_SEG09_POS) & SLCD_SEG08_11_SEG09_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg08_11_seg09(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value &= ~(SLCD_SEG08_11_SEG09_MASK << SLCD_SEG08_11_SEG09_POS);
    reg_value |= ((value & SLCD_SEG08_11_SEG09_MASK) << SLCD_SEG08_11_SEG09_POS);
    REG_WRITE(SLCD_SEG08_11_ADDR,reg_value);
}

/* REG_0x06:seg08_11->seg10:0x6[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg08_11_seg10(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value = ((reg_value >> SLCD_SEG08_11_SEG10_POS) & SLCD_SEG08_11_SEG10_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg08_11_seg10(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value &= ~(SLCD_SEG08_11_SEG10_MASK << SLCD_SEG08_11_SEG10_POS);
    reg_value |= ((value & SLCD_SEG08_11_SEG10_MASK) << SLCD_SEG08_11_SEG10_POS);
    REG_WRITE(SLCD_SEG08_11_ADDR,reg_value);
}

/* REG_0x06:seg08_11->seg11:0x6[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg08_11_seg11(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value = ((reg_value >> SLCD_SEG08_11_SEG11_POS) & SLCD_SEG08_11_SEG11_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg08_11_seg11(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG08_11_ADDR);
    reg_value &= ~(SLCD_SEG08_11_SEG11_MASK << SLCD_SEG08_11_SEG11_POS);
    reg_value |= ((value & SLCD_SEG08_11_SEG11_MASK) << SLCD_SEG08_11_SEG11_POS);
    REG_WRITE(SLCD_SEG08_11_ADDR,reg_value);
}

/* REG_0x07 */
#define SLCD_SEG12_15_ADDR  (SLCD_LL_REG_BASE  + 0x7*4) //REG ADDR :0x458e001c
#define SLCD_SEG12_15_SEG12_POS (0)
#define SLCD_SEG12_15_SEG12_MASK (0xFF)
#define SLCD_SEG12_15_SEG13_POS (8)
#define SLCD_SEG12_15_SEG13_MASK (0xFF)
#define SLCD_SEG12_15_SEG14_POS (16)
#define SLCD_SEG12_15_SEG14_MASK (0xFF)
#define SLCD_SEG12_15_SEG15_POS (24)
#define SLCD_SEG12_15_SEG15_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg12_15_value(void)
{
    return REG_READ(SLCD_SEG12_15_ADDR);
}

static inline void slcd_ll_set_seg12_15_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG12_15_ADDR,value);
}

/* REG_0x07:seg12_15->seg12:0x7[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg12_15_seg12(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value = ((reg_value >> SLCD_SEG12_15_SEG12_POS) & SLCD_SEG12_15_SEG12_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg12_15_seg12(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value &= ~(SLCD_SEG12_15_SEG12_MASK << SLCD_SEG12_15_SEG12_POS);
    reg_value |= ((value & SLCD_SEG12_15_SEG12_MASK) << SLCD_SEG12_15_SEG12_POS);
    REG_WRITE(SLCD_SEG12_15_ADDR,reg_value);
}

/* REG_0x07:seg12_15->seg13:0x7[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg12_15_seg13(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value = ((reg_value >> SLCD_SEG12_15_SEG13_POS) & SLCD_SEG12_15_SEG13_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg12_15_seg13(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value &= ~(SLCD_SEG12_15_SEG13_MASK << SLCD_SEG12_15_SEG13_POS);
    reg_value |= ((value & SLCD_SEG12_15_SEG13_MASK) << SLCD_SEG12_15_SEG13_POS);
    REG_WRITE(SLCD_SEG12_15_ADDR,reg_value);
}

/* REG_0x07:seg12_15->seg14:0x7[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg12_15_seg14(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value = ((reg_value >> SLCD_SEG12_15_SEG14_POS) & SLCD_SEG12_15_SEG14_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg12_15_seg14(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value &= ~(SLCD_SEG12_15_SEG14_MASK << SLCD_SEG12_15_SEG14_POS);
    reg_value |= ((value & SLCD_SEG12_15_SEG14_MASK) << SLCD_SEG12_15_SEG14_POS);
    REG_WRITE(SLCD_SEG12_15_ADDR,reg_value);
}

/* REG_0x07:seg12_15->seg15:0x7[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg12_15_seg15(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value = ((reg_value >> SLCD_SEG12_15_SEG15_POS) & SLCD_SEG12_15_SEG15_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg12_15_seg15(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG12_15_ADDR);
    reg_value &= ~(SLCD_SEG12_15_SEG15_MASK << SLCD_SEG12_15_SEG15_POS);
    reg_value |= ((value & SLCD_SEG12_15_SEG15_MASK) << SLCD_SEG12_15_SEG15_POS);
    REG_WRITE(SLCD_SEG12_15_ADDR,reg_value);
}

/* REG_0x08 */
#define SLCD_SEG16_19_ADDR  (SLCD_LL_REG_BASE  + 0x8*4) //REG ADDR :0x458e0020
#define SLCD_SEG16_19_SEG16_POS (0)
#define SLCD_SEG16_19_SEG16_MASK (0xFF)
#define SLCD_SEG16_19_SEG17_POS (8)
#define SLCD_SEG16_19_SEG17_MASK (0xFF)
#define SLCD_SEG16_19_SEG18_POS (16)
#define SLCD_SEG16_19_SEG18_MASK (0xFF)
#define SLCD_SEG16_19_SEG19_POS (24)
#define SLCD_SEG16_19_SEG19_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg16_19_value(void)
{
    return REG_READ(SLCD_SEG16_19_ADDR);
}

static inline void slcd_ll_set_seg16_19_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG16_19_ADDR,value);
}

/* REG_0x08:seg16_19->seg16:0x8[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg16_19_seg16(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value = ((reg_value >> SLCD_SEG16_19_SEG16_POS) & SLCD_SEG16_19_SEG16_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg16_19_seg16(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value &= ~(SLCD_SEG16_19_SEG16_MASK << SLCD_SEG16_19_SEG16_POS);
    reg_value |= ((value & SLCD_SEG16_19_SEG16_MASK) << SLCD_SEG16_19_SEG16_POS);
    REG_WRITE(SLCD_SEG16_19_ADDR,reg_value);
}

/* REG_0x08:seg16_19->seg17:0x8[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg16_19_seg17(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value = ((reg_value >> SLCD_SEG16_19_SEG17_POS) & SLCD_SEG16_19_SEG17_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg16_19_seg17(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value &= ~(SLCD_SEG16_19_SEG17_MASK << SLCD_SEG16_19_SEG17_POS);
    reg_value |= ((value & SLCD_SEG16_19_SEG17_MASK) << SLCD_SEG16_19_SEG17_POS);
    REG_WRITE(SLCD_SEG16_19_ADDR,reg_value);
}

/* REG_0x08:seg16_19->seg18:0x8[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg16_19_seg18(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value = ((reg_value >> SLCD_SEG16_19_SEG18_POS) & SLCD_SEG16_19_SEG18_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg16_19_seg18(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value &= ~(SLCD_SEG16_19_SEG18_MASK << SLCD_SEG16_19_SEG18_POS);
    reg_value |= ((value & SLCD_SEG16_19_SEG18_MASK) << SLCD_SEG16_19_SEG18_POS);
    REG_WRITE(SLCD_SEG16_19_ADDR,reg_value);
}

/* REG_0x08:seg16_19->seg19:0x8[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg16_19_seg19(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value = ((reg_value >> SLCD_SEG16_19_SEG19_POS) & SLCD_SEG16_19_SEG19_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg16_19_seg19(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG16_19_ADDR);
    reg_value &= ~(SLCD_SEG16_19_SEG19_MASK << SLCD_SEG16_19_SEG19_POS);
    reg_value |= ((value & SLCD_SEG16_19_SEG19_MASK) << SLCD_SEG16_19_SEG19_POS);
    REG_WRITE(SLCD_SEG16_19_ADDR,reg_value);
}

/* REG_0x09 */
#define SLCD_SEG20_23_ADDR  (SLCD_LL_REG_BASE  + 0x9*4) //REG ADDR :0x458e0024
#define SLCD_SEG20_23_SEG20_POS (0)
#define SLCD_SEG20_23_SEG20_MASK (0xFF)
#define SLCD_SEG20_23_SEG21_POS (8)
#define SLCD_SEG20_23_SEG21_MASK (0xFF)
#define SLCD_SEG20_23_SEG22_POS (16)
#define SLCD_SEG20_23_SEG22_MASK (0xFF)
#define SLCD_SEG20_23_SEG23_POS (24)
#define SLCD_SEG20_23_SEG23_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg20_23_value(void)
{
    return REG_READ(SLCD_SEG20_23_ADDR);
}

static inline void slcd_ll_set_seg20_23_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG20_23_ADDR,value);
}

/* REG_0x09:seg20_23->seg20:0x9[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg20_23_seg20(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value = ((reg_value >> SLCD_SEG20_23_SEG20_POS) & SLCD_SEG20_23_SEG20_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg20_23_seg20(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value &= ~(SLCD_SEG20_23_SEG20_MASK << SLCD_SEG20_23_SEG20_POS);
    reg_value |= ((value & SLCD_SEG20_23_SEG20_MASK) << SLCD_SEG20_23_SEG20_POS);
    REG_WRITE(SLCD_SEG20_23_ADDR,reg_value);
}

/* REG_0x09:seg20_23->seg21:0x9[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg20_23_seg21(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value = ((reg_value >> SLCD_SEG20_23_SEG21_POS) & SLCD_SEG20_23_SEG21_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg20_23_seg21(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value &= ~(SLCD_SEG20_23_SEG21_MASK << SLCD_SEG20_23_SEG21_POS);
    reg_value |= ((value & SLCD_SEG20_23_SEG21_MASK) << SLCD_SEG20_23_SEG21_POS);
    REG_WRITE(SLCD_SEG20_23_ADDR,reg_value);
}

/* REG_0x09:seg20_23->seg22:0x9[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg20_23_seg22(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value = ((reg_value >> SLCD_SEG20_23_SEG22_POS) & SLCD_SEG20_23_SEG22_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg20_23_seg22(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value &= ~(SLCD_SEG20_23_SEG22_MASK << SLCD_SEG20_23_SEG22_POS);
    reg_value |= ((value & SLCD_SEG20_23_SEG22_MASK) << SLCD_SEG20_23_SEG22_POS);
    REG_WRITE(SLCD_SEG20_23_ADDR,reg_value);
}

/* REG_0x09:seg20_23->seg23:0x9[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg20_23_seg23(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value = ((reg_value >> SLCD_SEG20_23_SEG23_POS) & SLCD_SEG20_23_SEG23_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg20_23_seg23(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG20_23_ADDR);
    reg_value &= ~(SLCD_SEG20_23_SEG23_MASK << SLCD_SEG20_23_SEG23_POS);
    reg_value |= ((value & SLCD_SEG20_23_SEG23_MASK) << SLCD_SEG20_23_SEG23_POS);
    REG_WRITE(SLCD_SEG20_23_ADDR,reg_value);
}

/* REG_0x0A */
#define SLCD_SEG24_27_ADDR  (SLCD_LL_REG_BASE  + 0xA*4) //REG ADDR :0x458e0028
#define SLCD_SEG24_27_SEG24_POS (0)
#define SLCD_SEG24_27_SEG24_MASK (0xFF)
#define SLCD_SEG24_27_SEG25_POS (8)
#define SLCD_SEG24_27_SEG25_MASK (0xFF)
#define SLCD_SEG24_27_SEG26_POS (16)
#define SLCD_SEG24_27_SEG26_MASK (0xFF)
#define SLCD_SEG24_27_SEG27_POS (24)
#define SLCD_SEG24_27_SEG27_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg24_27_value(void)
{
    return REG_READ(SLCD_SEG24_27_ADDR);
}

static inline void slcd_ll_set_seg24_27_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG24_27_ADDR,value);
}

/* REG_0x0A:seg24_27->seg24:0xA[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg24_27_seg24(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value = ((reg_value >> SLCD_SEG24_27_SEG24_POS) & SLCD_SEG24_27_SEG24_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg24_27_seg24(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value &= ~(SLCD_SEG24_27_SEG24_MASK << SLCD_SEG24_27_SEG24_POS);
    reg_value |= ((value & SLCD_SEG24_27_SEG24_MASK) << SLCD_SEG24_27_SEG24_POS);
    REG_WRITE(SLCD_SEG24_27_ADDR,reg_value);
}

/* REG_0x0A:seg24_27->seg25:0xA[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg24_27_seg25(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value = ((reg_value >> SLCD_SEG24_27_SEG25_POS) & SLCD_SEG24_27_SEG25_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg24_27_seg25(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value &= ~(SLCD_SEG24_27_SEG25_MASK << SLCD_SEG24_27_SEG25_POS);
    reg_value |= ((value & SLCD_SEG24_27_SEG25_MASK) << SLCD_SEG24_27_SEG25_POS);
    REG_WRITE(SLCD_SEG24_27_ADDR,reg_value);
}

/* REG_0x0A:seg24_27->seg26:0xA[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg24_27_seg26(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value = ((reg_value >> SLCD_SEG24_27_SEG26_POS) & SLCD_SEG24_27_SEG26_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg24_27_seg26(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value &= ~(SLCD_SEG24_27_SEG26_MASK << SLCD_SEG24_27_SEG26_POS);
    reg_value |= ((value & SLCD_SEG24_27_SEG26_MASK) << SLCD_SEG24_27_SEG26_POS);
    REG_WRITE(SLCD_SEG24_27_ADDR,reg_value);
}

/* REG_0x0A:seg24_27->seg27:0xA[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg24_27_seg27(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value = ((reg_value >> SLCD_SEG24_27_SEG27_POS) & SLCD_SEG24_27_SEG27_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg24_27_seg27(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG24_27_ADDR);
    reg_value &= ~(SLCD_SEG24_27_SEG27_MASK << SLCD_SEG24_27_SEG27_POS);
    reg_value |= ((value & SLCD_SEG24_27_SEG27_MASK) << SLCD_SEG24_27_SEG27_POS);
    REG_WRITE(SLCD_SEG24_27_ADDR,reg_value);
}

/* REG_0x0B */
#define SLCD_SEG28_31_ADDR  (SLCD_LL_REG_BASE  + 0xB*4) //REG ADDR :0x458e002c
#define SLCD_SEG28_31_SEG28_POS (0)
#define SLCD_SEG28_31_SEG28_MASK (0xFF)
#define SLCD_SEG28_31_SEG29_POS (8)
#define SLCD_SEG28_31_SEG29_MASK (0xFF)
#define SLCD_SEG28_31_SEG30_POS (16)
#define SLCD_SEG28_31_SEG30_MASK (0xFF)
#define SLCD_SEG28_31_SEG31_POS (24)
#define SLCD_SEG28_31_SEG31_MASK (0xFF)

static inline uint32_t slcd_ll_get_seg28_31_value(void)
{
    return REG_READ(SLCD_SEG28_31_ADDR);
}

static inline void slcd_ll_set_seg28_31_value(uint32_t value)
{
    REG_WRITE(SLCD_SEG28_31_ADDR,value);
}

/* REG_0x0B:seg28_31->seg28:0xB[7:0],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg28_31_seg28(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value = ((reg_value >> SLCD_SEG28_31_SEG28_POS) & SLCD_SEG28_31_SEG28_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg28_31_seg28(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value &= ~(SLCD_SEG28_31_SEG28_MASK << SLCD_SEG28_31_SEG28_POS);
    reg_value |= ((value & SLCD_SEG28_31_SEG28_MASK) << SLCD_SEG28_31_SEG28_POS);
    REG_WRITE(SLCD_SEG28_31_ADDR,reg_value);
}

/* REG_0x0B:seg28_31->seg29:0xB[15:8],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg28_31_seg29(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value = ((reg_value >> SLCD_SEG28_31_SEG29_POS) & SLCD_SEG28_31_SEG29_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg28_31_seg29(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value &= ~(SLCD_SEG28_31_SEG29_MASK << SLCD_SEG28_31_SEG29_POS);
    reg_value |= ((value & SLCD_SEG28_31_SEG29_MASK) << SLCD_SEG28_31_SEG29_POS);
    REG_WRITE(SLCD_SEG28_31_ADDR,reg_value);
}

/* REG_0x0B:seg28_31->seg30:0xB[23:16],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg28_31_seg30(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value = ((reg_value >> SLCD_SEG28_31_SEG30_POS) & SLCD_SEG28_31_SEG30_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg28_31_seg30(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value &= ~(SLCD_SEG28_31_SEG30_MASK << SLCD_SEG28_31_SEG30_POS);
    reg_value |= ((value & SLCD_SEG28_31_SEG30_MASK) << SLCD_SEG28_31_SEG30_POS);
    REG_WRITE(SLCD_SEG28_31_ADDR,reg_value);
}

/* REG_0x0B:seg28_31->seg31:0xB[31:24],0x0,R/W*/
static inline uint32_t slcd_ll_get_seg28_31_seg31(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value = ((reg_value >> SLCD_SEG28_31_SEG31_POS) & SLCD_SEG28_31_SEG31_MASK);
    return reg_value;
}

static inline void slcd_ll_set_seg28_31_seg31(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_SEG28_31_ADDR);
    reg_value &= ~(SLCD_SEG28_31_SEG31_MASK << SLCD_SEG28_31_SEG31_POS);
    reg_value |= ((value & SLCD_SEG28_31_SEG31_MASK) << SLCD_SEG28_31_SEG31_POS);
    REG_WRITE(SLCD_SEG28_31_ADDR,reg_value);
}

/* REG_0x0C */
#define SLCD_CRTL_REG_ADDR  (SLCD_LL_REG_BASE  + 0xC*4) //REG ADDR :0x458e0030
#define SLCD_CRTL_REG_LCD_ON_POS (0)
#define SLCD_CRTL_REG_LCD_ON_MASK (0x1)
#define SLCD_CRTL_REG_COM_NUMBER_POS (1)
#define SLCD_CRTL_REG_COM_NUMBER_MASK (0x1)
#define SLCD_CRTL_REG_LCD_BIAS_POS (2)
#define SLCD_CRTL_REG_LCD_BIAS_MASK (0x1)
#define SLCD_CRTL_REG_RESERVED0_POS (3)
#define SLCD_CRTL_REG_RESERVED0_MASK (0x1)
#define SLCD_CRTL_REG_LCD_RATE_POS (4)
#define SLCD_CRTL_REG_LCD_RATE_MASK (0x3)
#define SLCD_CRTL_REG_RESERVED1_POS (6)
#define SLCD_CRTL_REG_RESERVED1_MASK (0x3FFFFFF)

static inline uint32_t slcd_ll_get_ctrl_reg_value(void)
{
    return REG_READ(SLCD_CRTL_REG_ADDR);
}

static inline void slcd_ll_set_ctrl_reg_value(uint32_t value)
{
    REG_WRITE(SLCD_CRTL_REG_ADDR,value);
}

/* REG_0x0C:ctrl_reg->lcd_on:0xC[0],0x0,R/W*/
static inline uint32_t slcd_ll_get_ctrl_reg_lcd_on(void)
{
    return REG_READ(SLCD_CRTL_REG_ADDR);
}

static inline void slcd_ll_set_ctrl_reg_lcd_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CRTL_REG_ADDR);
    reg_value &= ~(SLCD_CRTL_REG_LCD_ON_MASK << SLCD_CRTL_REG_LCD_ON_POS);
    reg_value |= ((value & SLCD_CRTL_REG_LCD_ON_MASK) << SLCD_CRTL_REG_LCD_ON_POS);
    REG_WRITE(SLCD_CRTL_REG_ADDR,reg_value);
}

/* REG_0x0C:ctrl_reg->com_number:0xC[1],0x0,R/W*/
static inline uint32_t slcd_ll_get_ctrl_reg_com_number(void)
{
    return REG_READ(SLCD_CRTL_REG_ADDR);
}

static inline void slcd_ll_set_ctrl_reg_com_number(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CRTL_REG_ADDR);
    reg_value &= ~(SLCD_CRTL_REG_COM_NUMBER_MASK << SLCD_CRTL_REG_COM_NUMBER_POS);
    reg_value |= ((value & SLCD_CRTL_REG_COM_NUMBER_MASK) << SLCD_CRTL_REG_COM_NUMBER_POS);
    REG_WRITE(SLCD_CRTL_REG_ADDR,reg_value);
}

/* REG_0x0C:ctrl_reg->lcd_bias:0xC[2],0x0,R/W*/
static inline uint32_t slcd_ll_get_ctrl_reg_lcd_bias(void)
{
    return REG_READ(SLCD_CRTL_REG_ADDR);
}

static inline void slcd_ll_set_ctrl_reg_lcd_bias(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CRTL_REG_ADDR);
    reg_value &= ~(SLCD_CRTL_REG_LCD_BIAS_MASK << SLCD_CRTL_REG_LCD_BIAS_POS);
    reg_value |= ((value & SLCD_CRTL_REG_LCD_BIAS_MASK) << SLCD_CRTL_REG_LCD_BIAS_POS);
    REG_WRITE(SLCD_CRTL_REG_ADDR,reg_value);
}

/* REG_0x0C:ctrl_reg->lcd_rate:0xC[5:4],0x0,R/W*/
static inline uint32_t slcd_ll_get_ctrl_reg_lcd_rate(void)
{
    return REG_READ(SLCD_CRTL_REG_ADDR);
}

static inline void slcd_ll_set_ctrl_reg_lcd_rate(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(SLCD_CRTL_REG_ADDR);
    reg_value &= ~(SLCD_CRTL_REG_LCD_RATE_MASK << SLCD_CRTL_REG_LCD_RATE_POS);
    reg_value |= ((value & SLCD_CRTL_REG_LCD_RATE_MASK) << SLCD_CRTL_REG_LCD_RATE_POS);
    REG_WRITE(SLCD_CRTL_REG_ADDR,reg_value);
}


#ifdef __cplusplus
}
#endif

