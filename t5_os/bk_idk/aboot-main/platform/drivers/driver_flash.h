/*************************************************************
 * @file        driver_flash.h
 * @brief       Header file of driver_flash.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_FLASH_H__
#define __DRIVER_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "BK_config.h"


#if (CFG_SOC_NAME == SOC_BK7256)
#define REG_FLASH_BASE_ADDR                 (0x44030000UL)
#else
#define REG_FLASH_BASE_ADDR                 (0x00803000UL)
#endif

#ifdef FLASH_3231_STYLE
#define REG_FLASH_OPERATE_SW_ADDR           (REG_FLASH_BASE_ADDR + 0 * 4)
#define REG_FLASH_OPERATE_SW_MASK           0xFFFFFFFFUL
#define REG_FLASH_OPERATE_SW                (*((volatile unsigned long *) REG_FLASH_OPERATE_SW_ADDR))

#define FLASH_OPERATE_SW_OP_ADDR_SW_POSI    0
#define FLASH_OPERATE_SW_OP_ADDR_SW_MASK    (0x00FFFFFFUL << FLASH_OPERATE_SW_OP_ADDR_SW_POSI)
#define FLASH_OPERATE_SW_OP_ADDR_SW_SET     (0x00FFFFFFUL << FLASH_OPERATE_SW_OP_ADDR_SW_POSI)

#define FLASH_OPERATE_SW_OP_TYPE_SW_POSI    24
#define FLASH_OPERATE_SW_OP_TYPE_SW_MASK    (0x1FUL << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)
#define FLASH_OPERATE_SW_OP_TYPE_SW_SET     (0x1FUL << FLASH_OPERATE_SW_OP_TYPE_SW_POSI)

#define FLASH_OPERATE_SW_OP_START_POSI      29
#define FLASH_OPERATE_SW_OP_START_MASK      (0x01UL << FLASH_OPERATE_SW_OP_START_POSI)
#define FLASH_OPERATE_SW_OP_START_SET       (0x01UL << FLASH_OPERATE_SW_OP_START_POSI)

#define FLASH_OPERATE_SW_WP_VALUE_POSI      30
#define FLASH_OPERATE_SW_WP_VALUE_MASK      (0x01UL << FLASH_OPERATE_SW_WP_VALUE_POSI)
#define FLASH_OPERATE_SW_WP_VALUE_SET       (0x01UL << FLASH_OPERATE_SW_WP_VALUE_POSI)

#define FLASH_OPERATE_SW_BUSY_SW_POSI       31
#define FLASH_OPERATE_SW_BUSY_SW_MASK       (0x01UL << FLASH_OPERATE_SW_BUSY_SW_POSI)
#define FLASH_OPERATE_SW_BUSY_SW_SET        (0x01UL << FLASH_OPERATE_SW_BUSY_SW_POSI)


#define REG_FLASH_DATA_WRITE_FLASH_ADDR     (REG_FLASH_BASE_ADDR + 1 * 4)
#define REG_FLASH_DATA_WRITE_FLASH_MASK     0xFFFFFFFFUL
#define REG_FLASH_DATA_WRITE_FLASH          (*((volatile unsigned long *) REG_FLASH_DATA_WRITE_FLASH_ADDR))


#define REG_FLASH_DATA_READ_FLASH_ADDR      (REG_FLASH_BASE_ADDR + 2 * 4)
#define REG_FLASH_DATA_READ_FLASH_MASK      0xFFFFFFFFUL
#define REG_FLASH_DATA_READ_FLASH           (*((volatile unsigned long *) REG_FLASH_DATA_READ_FLASH_ADDR))


#define REG_FLASH_READ_ID_DATA_ADDR         (REG_FLASH_BASE_ADDR + 4 * 4)
#define REG_FLASH_READ_ID_DATA_MASK         0xFFFFFFFFUL
#define REG_FLASH_READ_ID_DATA              (*((volatile unsigned long *) REG_FLASH_READ_ID_DATA_ADDR))


#define REG_FLASH_CRC_CHECK_ADDR            (REG_FLASH_BASE_ADDR + 5 * 4)
#define REG_FLASH_CRC_CHECK_MASK            0x003FFFFFUL
#define REG_FLASH_CRC_CHECK                 (*((volatile unsigned long *) REG_FLASH_CRC_CHECK_ADDR))

#define FLASH_CRC_CHECK_SR_DATA_READ_POSI   0
#define FLASH_CRC_CHECK_SR_DATA_READ_MASK   (0x00FFUL << FLASH_CRC_CHECK_SR_DATA_READ_POSI)
#define FLASH_CRC_CHECK_SR_DATA_READ_SET    (0x00FFUL << FLASH_CRC_CHECK_SR_DATA_READ_POSI)

#define FLASH_CRC_CHECK_CRC_ERR_CNT_POSI    8
#define FLASH_CRC_CHECK_CRC_ERR_CNT_MASK    (0x00FFUL << FLASH_CRC_CHECK_CRC_ERR_CNT_POSI)
#define FLASH_CRC_CHECK_CRC_ERR_CNT_SET     (0x00FFUL << FLASH_CRC_CHECK_CRC_ERR_CNT_POSI)

#define FLASH_CRC_CHECK_DATA_WRITE_CNT_POSI 16
#define FLASH_CRC_CHECK_DATA_WRITE_CNT_MASK (0x07UL << FLASH_CRC_CHECK_DATA_WRITE_CNT_POSI)
#define FLASH_CRC_CHECK_DATA_WRITE_CNT_SET  (0x07UL << FLASH_CRC_CHECK_DATA_WRITE_CNT_POSI)

#define FLASH_CRC_CHECK_DATA_READ_CNT_POSI  19
#define FLASH_CRC_CHECK_DATA_READ_CNT_MASK  (0x07UL << FLASH_CRC_CHECK_DATA_READ_CNT_POSI)
#define FLASH_CRC_CHECK_DATA_READ_CNT_SET   (0x07UL << FLASH_CRC_CHECK_DATA_READ_CNT_POSI)


#define REG_FLASH_CONFIG_ADDR               (REG_FLASH_BASE_ADDR + 7 * 4)
#define REG_FLASH_CONFIG_MASK               0x07FFFFFFUL
#define REG_FLASH_CONFIG                    (*((volatile unsigned long *) REG_FLASH_CONFIG_ADDR))

#define FLASH_CONFIG_CPU_WRITE_ENABLE_POSI          9
#define FLASH_CONFIG_CPU_WRITE_ENABLE_MASK   (0x1FUL << FLASH_CONFIG_CPU_WRITE_ENABLE_POSI)

#define FLASH_CONFIG_CLK_CONF_POSI          0
#define FLASH_CONFIG_CLK_CONF_MASK          (0x0FUL << FLASH_CONFIG_CLK_CONF_POSI)
#define FLASH_CONFIG_CLK_CONF_SET           (0x0FUL << FLASH_CONFIG_CLK_CONF_POSI)

#define FLASH_CONFIG_MODE_SEL_POSI          4
#define FLASH_CONFIG_MODE_SEL_MASK          (0x1FUL << FLASH_CONFIG_MODE_SEL_POSI)
#define FLASH_CONFIG_MODE_SEL_SET           (0x1FUL << FLASH_CONFIG_MODE_SEL_POSI)

#define FLASH_CONFIG_FWREN_FLASH_CPU_POSI   9
#define FLASH_CONFIG_FWREN_FLASH_CPU_MASK   (0x01UL << FLASH_CONFIG_FWREN_FLASH_CPU_POSI)
#define FLASH_CONFIG_FWREN_FLASH_CPU_SET    (0x01UL << FLASH_CONFIG_FWREN_FLASH_CPU_POSI)

#define FLASH_CONFIG_WRSR_DATA_POSI         10
#define FLASH_CONFIG_WRSR_DATA_MASK         (0x00FFFFUL << FLASH_CONFIG_WRSR_DATA_POSI)
#define FLASH_CONFIG_WRSR_DATA_SET          (0x00FFFFUL << FLASH_CONFIG_WRSR_DATA_POSI)

#define FLASH_CONFIG_CRC_EN_POSI            26
#define FLASH_CONFIG_CRC_EN_MASK            (0x01UL << FLASH_CONFIG_CRC_EN_POSI)
#define FLASH_CONFIG_CRC_EN_SET             (0x01UL << FLASH_CONFIG_CRC_EN_POSI)
#endif      /* #ifdef FLASH_3231_STYLE */


// flash operation command type(decimal)
typedef enum {
    FLASH_OPCODE_WREN    = 1,
    FLASH_OPCODE_WRDI    = 2,
    FLASH_OPCODE_RDSR    = 3,
    FLASH_OPCODE_WRSR    = 4,
    FLASH_OPCODE_READ    = 5,
    FLASH_OPCODE_RDSR2   = 6,
    FLASH_OPCODE_WRSR2   = 7,
    FLASH_OPCODE_PP      = 12,
    FLASH_OPCODE_SE      = 13,
    FLASH_OPCODE_BE1     = 14,
    FLASH_OPCODE_BE2     = 15,
    FLASH_OPCODE_CE      = 16,
    FLASH_OPCODE_DP      = 17,
    FLASH_OPCODE_RFDP    = 18,
    FLASH_OPCODE_RDID    = 20,
    FLASH_OPCODE_HPM     = 21,
    FLASH_OPCODE_CRMR    = 22,
    FLASH_OPCODE_CRMR2   = 23
}FLASH_OPCODE;

typedef struct
{
    u32 flash_id;
    u8  sr_size;
    u16  protect_all;
    u16  protect_none;
} flash_config_t;

typedef enum
{
    NONE,
    ALL
} PROTECT_TYPE;

extern void set_flash_clk(unsigned char clk_conf);
extern void set_flash_qe(void);
extern void set_flash_wsr(unsigned short value);
extern void flash_set_line_mode(unsigned char mode);
extern void flash_set_clk(unsigned char clk);
extern unsigned long flash_read_mID(void);
extern void flash_erase_sector(unsigned long address);
extern void flash_erase_block(unsigned long address);
extern void flash_read_data(unsigned char *buffer, unsigned long address, unsigned long len);
extern void flash_write_data(unsigned char *buffer, unsigned long address, unsigned long len);
extern void SET_FLASH_HPM(void);

extern void flash_init(void);
extern void flash_set_16M_1line(void);
extern void flash_set_96M_4line(void);

void bk_flash_lock(void);
void bk_flash_unlock(void);

extern void clr_flash_protect(void);
extern void get_flash_ID(void);
extern void flash_get_current_flash_config(void);
extern void set_flash_protect(PROTECT_TYPE type);
extern int flash_erase(long addr, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DRIVER_FLASH_H__ */
