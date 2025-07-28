#ifndef __FLASH_BYPASS_H__
#define __FLASH_BYPASS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <components/log.h>

#define FLASH_BYPASS_TAG "flash_bypass"
#define FLASH_BYPASS_LOGI(...) BK_LOGI(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGW(...) BK_LOGW(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGE(...) BK_LOGE(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGD(...) BK_LOGD(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_LOGV(...) BK_LOGV(FLASH_BYPASS_TAG, ##__VA_ARGS__)

#define FLASH_BYPASS_RAWI(...) BK_RAW_LOGI(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWW(...) BK_RAW_LOGW(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWE(...) BK_RAW_LOGE(FLASH_BYPASS_TAG, ##__VA_ARGS__)
#define FLASH_BYPASS_RAWD(...) BK_RAW_LOGD(FLASH_BYPASS_TAG, ##__VA_ARGS__)

#define CURRENT_ROUTINE_TEXT_SIZE       (0x400)

/*Write Enable for Status Register*/
#define FLASH_CMD_WR_EN_SR              (0x6)
#define FLASH_CMD_WR_DISABLE            (0x04)

/*Write Status Register*/
#define FLASH_CMD_WR_SR                 (0x31)

/*Others*/
#define FLASH_CMD_QUAD_IO_FAST_READ     (0xEB)
#define FLASH_GD25Q32C_SR_QUAD_EN       (0x2)
#define FLASH_CMD_ENTER_DEEP_PWR_DW     (0xB9)
#define FLASH_CMD_EXIT_DEEP_PWR_DW      (0xAB)

/*SYS regs define*/
#define SYS_R_ADD_X(x)                  (0x44010000+((x)*4))

/*SPI0 regs define*/
#define SPI_CFG_TRX_LEN_MASK            (0xffffU)
#define SPI_CFG_TX_TRAHS_LEN_POSI       (8)
#define SPI_CFG_RX_TRAHS_LEN_POSI       (20)
#define SPI_CFG_RX_FIN_INT_EN           (BIT(3))
#define SPI_CFG_TX_FIN_INT_EN           (BIT(2))
#define SPI_CFG_RX_EN                   (BIT(1))
#define SPI_CFG_TX_EN                   (BIT(0))
#define SPI_CFG_TX_EN_ONE_BYTE          (0x10D)
#define SPI_CFG_TX_EN_TWO_BYTE          (0x20D)
#define SPI_STATUS_TX_FINISH_INT        (BIT(13))
#define SPI_STATUS_RX_FINISH_INT        (BIT(14))
#define SPI_STATUS_TXFIFO_WR_READY      (BIT(1))
#define SPI_STATUS_RXFIFO_RD_READY      (BIT(2))

#define FLASH_ID_GD25Q32C               (0xC84016)
#define FLASH_ID_TH25Q64                (0xCD6017)
#define QE_RETRY_TIMES                  (10)

#if CONFIG_SOC_BK7236XX
__attribute__((section(".itcm_sec_code"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len);
#else
__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_enable(void);
__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_test(uint32_t quad_enable, uint32_t delay_cycle1, uint32_t delay_cycle2);
#endif

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
#define FLASH_BYPASS_OTP_BLOCK_START_OFFSET    (0)
#define FLASH_BYPASS_OTP_BLOCK_LENGTH          (1024)

#define FLASH_BYPASS_OTP_PAGE_LENGTH           (256)

#define FLASH_BYPASS_OTP_BLOCK_NUM             (3)

#define FLASH_BYPASS_OTP_IDX_X_ADDR(x)         ((x) << 12)

#define CMD_FLASH_BYPASS_WR_EN                 (0x06)
#define CMD_FLASH_BYPASS_WR_DI                 (0x04)

#define CMD_FLASH_BYPASS_OTP_READ_TASK_LEN     (5)
#define CMD_FLASH_BYPASS_OTP_READ              (0x48)

#define CMD_FLASH_BYPASS_OTP_EARSE_TASK_LEN    (4)
#define CMD_FLASH_BYPASS_OTP_EARSE             (0x44)

#define CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN    (4)
#define CMD_FLASH_BYPASS_OTP_WRITE_TASK_LEN    (CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN + FLASH_BYPASS_OTP_BLOCK_LENGTH)
#define CMD_FLASH_BYPASS_OTP_WRITE             (0x42)

#define CMD_FLASH_STA_REG_1_READ               (0x05)
#define CMD_FLASH_STA_REG_2_READ               (0x35)
#define CMD_FLASH_STA_REG_3_READ               (0x15)
#define CMD_FLASH_STA_REG_1_WRITE              (0x01)
#define CMD_FLASH_STA_REG_2_WRITE              (0x31)
#define CMD_FLASH_STA_REG_3_WRITE              (0x11)

#define CMD_FLASH_STA_REG_READ_TASK_LEN        (1)
#define CMD_FLASH_STA_REG_WRITE_TASK_LEN       (2)

#define FLASH_STA_REG_LOCK_OTP_OFFSET          (10)
#define FLASH_STA_REG_WIP_BIT                  (1 << 0)
#define FLASH_STA_REG_QUAD_ENABLE_BIT          (1 << 9)

#define FLASH_BYPASS_OTP_READ_RETRY_MAX        (3)
#define FLASH_BYPASS_OTP_EARSE_RETRY_MAX       (3)
#define FLASH_BYPASS_OTP_LOCK_RETRY_MAX        (3)

typedef enum
{
	FLASH_BYPASS_OTP_READ = 1,
	FLASH_BYPASS_OTP_EARSE,
	FLASH_BYPASS_OTP_WRITE,
	FLASH_BYPASS_OTP_LOCK,
} flash_bypass_otp_cmd_t;

typedef enum
{
	FLASH_BYPASS_OTP_IDX_1 = 1,
	FLASH_BYPASS_OTP_IDX_2,
	FLASH_BYPASS_OTP_IDX_3,

	FLASH_BYPASS_OTP_IDX_MAX,
} flash_bypass_otp_idx_t;

typedef enum
{
	FLASH_STA_REG_1 = 0,
	FLASH_STA_REG_2,
	FLASH_STA_REG_3,

	FLASH_STA_REG_MAX,
} flash_bypass_otp_reg_idx_t;

typedef struct {
	flash_bypass_otp_idx_t otp_idx;
	uint32_t               addr_offset;
	uint8_t                *read_buf;
	uint32_t               read_len;
	uint8_t                *write_buf;
	uint32_t               write_len;
} flash_bypass_otp_ctrl_t;

bk_err_t flash_bypass_otp_operation(flash_bypass_otp_cmd_t cmd, flash_bypass_otp_ctrl_t *param);
#endif

#ifdef __cplusplus
}
#endif
#endif //__FLASH_BYPASS_H__
// eof

