#include "type.h"
#include "bl_config.h"
#include "system.h"
#include "bootloader.h"
#include "bl_extflash.h"
#include "bl_spi.h"
#include "bl_uart.h"
#include "pal_log.h"
#include "hal_platform.h"
#include "hal.h"
#if (CPU_RISCV)
#include "int.h"
#endif

#if (CPU_ARM)
#include "intc.h"
#endif

#if (CPU_BA22)
#include "exception.h"
#include "spr_defs.h"
#endif

#include "string.h"
#include "flash.h"
#include "bk_flash.h"
#include "bk7236_layout.h"
#include "otp.h"
#include "trng.h"
#include <stdio.h>
#include "efuse_hal.h"
/***************************************************CHANGE ***************************/


//#define cpu1_vector    (*((volatile unsigned long *)0x44010014))


void BK3000_ICU_Initial(void)
{
#if (CHIP_BK3281)
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_SPI_PWD;

    BK3000_PMU_PERINT_ENABLE = 0x2;
#endif
	
#if (CHIP_BK3266)
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_SPI_PWD;
//	BK3000_PMU_PERI_PWDS &= ~bit_PMU_CEVA_PWD; // enable CEVA baseband clock
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_STEAL_ENABLE; // Clock Stealing Enable to get 32k clock
	BK3000_PMU_PERI_PWDS |= bit_PMU_LPO_CLK_SEL_XTAL; // 1:XTAL?? 0:ROSC?? 32.768
	BK3000_PMU_PERI_PWDS &= ~bit_PMU_LPO_CLK_DISABLE; // enable LPO clock using for 32K clock
	BK3000_PMU_PERI_PWDS |= bit_PMU_CLK_FREQ_SEL;

#endif
	
#if (CHIP_BK3288)

	SET_PERIxCLK(UART0CLK_PWD)         ;
	SET_PERIxCLK(SPI0CLK_PWD)          ;
	addSYSTEM_Reg0x9 |=  INT_ID_UART0  ; 
	set_spi_rw_flash                   ;

	clr_ram_sd(uart0_ram_sd);
	clr_ram_sd(spi0_ram_sd);
#endif

#if (CHIP_BK3633)

UART0_ENABLE      ;
SPI0_ENABLE       ;
UART0_INT_ENABLE  ;
SET_SPI_RW_FLASH  ;

#endif


#if (CHIP_BK7256)

UART0_ENABLE      ;
SPI0_ENABLE       ;
UART0_INT_ENABLE  ;
SET_SPI_RW_FLASH  ;

#endif

#if (CHIP_BK7236)
	UART0_RESET;
	UART0_ENABLE;
	SPI0_ENABLE;
	UART0_INT_ENABLE;
	//SET_SPI_RW_FLASH;
#endif
}


void control_set_to_flash_not_enable_jtag()
{
#if (CHIP_BK3281)
	 BK3000_PMU_ROM_FLASH_SECT = 0;
#endif

#if (CHIP_BK3266)
	 BK3000_PMU_ROM_FLASH_SECT |= ( 0X01 << 0);
#endif

#if (CHIP_BK3288)
	 BK3000_PMU_ROM_FLASH_SECT &= ~(0x01<<0) ;
#endif
	
#if (CHIP_BK3633)	
	SET_FLASHCTRL_RW_FLASH ;
	REBOOT ;
#endif	

#if (CHIP_BK7256)
	SET_FLASHCTRL_RW_FLASH ;
	REBOOT ;
#endif

#if (CHIP_BK7236)
	SET_FLASHCTRL_RW_FLASH ;
	REBOOT ;
#endif
}

void control_set_to_flash()
{
	hal_jtag_enable();
	control_set_to_flash_not_enable_jtag();
}

extern u16 COUNT;
void uart_enable(uint8_t flag,uint8_t rx_thr) {

	if (flag == 0)
		boot_uart_init(UART_BAUDRATE_DEFAULE,rx_thr);
	else
		boot_uart_init(UART_BAUDRATE_DEFAULE,rx_thr);//UART_BAUDRATE_2000000

#if (CHIP_BK3281)
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 3 << 3; // only cfg Rx
#endif
	
#if (CHIP_BK3266)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3; // only cfg Rx
#endif

#if (CHIP_BK3288)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif
	
#if (CHIP_BK3633)
		BK3000_GPIO_1_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif	

#if (CHIP_BK7256)
		//BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
		BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif

#if (CHIP_BK7236)
	BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 0xf << 2; // only cfg Rx
#endif
}


void uart_tx_pin_cfg(void)
{
#if (CHIP_BK3281)
	BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE ;
#endif
	
#if (CHIP_BK3266)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
	
#if (CHIP_BK3288)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
	
#if (CHIP_BK3633)
	BK3000_GPIO_0_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif	

#if (CHIP_BK7256)
	//BK3000_GPIO_10_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif
#if (CHIP_BK7236)
	BK3000_GPIO_11_CONFIG = 1 << sft_GPIO_FUNCTION_ENABLE | 7 << 3;// only cfg tx
#endif

}



void PMU_uninit( void )
{
#if (CHIP_BK3266)
	BK3000_PMU_PERI_PWDS |= bit_PMU_UART_PWD;
	BK3000_PMU_PERI_PWDS |= bit_PMU_SPI_PWD;
#endif

#if (CHIP_BK3288)
	CLR_PERIxCLK(UART0CLK_PWD)           ;
	CLR_PERIxCLK(SPI0CLK_PWD)            ;
	addSYSTEM_Reg0x9 &=  ~ INT_ID_UART0  ;
	set_flashctrl_rw_flash               ;
	set_ram_sd(uart0_ram_sd)             ;
	set_ram_sd(spi0_ram_sd)              ;
#endif
	
#if (CHIP_BK3633)	
	 UART0_DISABLE      ;
	 SPI0_DISABLE       ;
	 UART0_INT_DISABLE  ;
//	 IRQ_DISABLE        ;
#else
	 UART0_DISABLE      ;
	 SPI0_DISABLE       ;
	 UART0_INT_DISABLE  ;
#endif		
	
}


void BK3000_start_wdt(uint32_t val) {
	
#if (CHIP_BK3633)	
	AON_WDT_PERIOD  = val  ;
	AON_WDT_FEED    = 0x5A ;
	AON_WDT_FEED    = 0xA5 ;	
#elif (CHIP_BK7256)
	BK3000_WDT_CONFIG = 0x5A0000 | val;
	BK3000_WDT_CONFIG = 0xA50000 | val;
	AON_WDT_CONFIG = 0x5A0000 | val;
	AON_WDT_CONFIG = 0xA50000 | val;
 #elif (CHIP_BK7236)
	/* On FPGA, the WDT clock is 25Mhz, it will trigger watchdog soon,
	 * since FPGA doesn't support reset, it triggers NMI instead of
	 * WDT reset.
	 * */
#if CONFIG_WDT
	BK3000_WDT_CONTROL = 0x3;
	BK3000_WDT_CONFIG = 0x5A0000 | val;
	BK3000_WDT_CONFIG = 0xA50000 | val;

	AON_WDT_CONFIG = 0x5A0000 | val;
	AON_WDT_CONFIG = 0xA50000 | val;
#endif
#endif	
}

/************************************CHANGE ***************************************************/
/***************************************************************************************/
void uart_disable(void) {
	REG_APB3_UART_CFG = 0;
}

void SYSirq_Initialise(void) {
	extern void arch_enable_uart_int(void);
	arch_enable_uart_int();
}

void bl_init(void) {
	bsp_initial();
	SYSirq_Initialise();
}

#ifdef FLASH_DEBUG
void set_flash_clk(unsigned char clk_conf) {
    unsigned int temp0;
    temp0 = reg_FLASH_CONF;
    reg_FLASH_CONF = (  (clk_conf << BIT_FLASH_CLK_CONF)
                      | (temp0    &  SET_MODE_SEL)
                      | (temp0    &  SET_FWREN_FLASH_CPU)
                      | (temp0    &  SET_WRSR_DATA)
                      | (temp0    &  SET_CRC_EN));
}
#endif

void watchdog_stop(void) {
	BK3000_WDT_CONFIG = 0x5A0000;
	BK3000_WDT_CONFIG = 0xA50000;
	return;
}

void BK3000_ICU_Initial(void);
void bsp_initial(void)
{

	BK3000_ICU_Initial();

}
extern u8 uart_link_check_flag ;

#define addEFUSE_Reg0x2                                         *((volatile unsigned long *) (0x44880000+0x2*4))
#define addEFUSE_Reg0x4                                         *((volatile unsigned long *) (0x44880000+0x4*4))
#define addEFUSE_Reg0x5                                         *((volatile unsigned long *) (0x44880000+0x5*4))
#define get_EFUSE_Reg0x1_efuse_read_data_valid                  ((addEFUSE_Reg0x5 & 0x100) >> 8)

void deep_sleep_to_flash()
{
	PAL_LOG_DEBUG("deep sleep rst, pmu reg0=%x\n", BK3000_PMU_RESET_REASON_GET);
	if( DEEP_SLEEP_RESTART )//restart from deepsleep ,so go to flash
	{
#if(CHIP_BK7256)
    		addEFUSE_Reg0x0 = 0x01 | (0x1e << 8);
    		while(get_EFUSE_Reg0x1_efuse_read_data_valid == 0);

		if( (addEFUSE_Reg0x1 & 0x4) == 0 ){    //efuse 0x1e[2] 0: deepsleep fast boot 1: deepsleep sb boot
    			control_set_to_flash();
    		}
#elif(CHIP_BK7236)
		addEFUSE_Reg0x2 = 0x03;
		addEFUSE_Reg0x4 = 0x01 | (0x0 << 8);
		while(get_EFUSE_Reg0x1_efuse_read_data_valid == 0);

		if( (addEFUSE_Reg0x5 & 0x4) == 0 ){    //efuse 0x0[2] 0: deepsleep fast boot 1: deepsleep sb boot
			control_set_to_flash();
		}
#else
		control_set_to_flash() ;
#endif
	}
}

#include "boot_protocol.h"

#define NORMAL_BOOTLOADER_PHY_START_ADDR    (0x0)
//#define NORMAL_BOOTLOADER_PHY_START_ADDR    (0x33000)
#define AES_BOOTLOADER_PHY_START_ADDR       (0x231000)
#define AES_BOOTLOADER_PHY_LENGTH           (0x22000)
#define FLASH_SECTOR_LENGTH                 (0x1000)

#define NORMAL_BOOTLOADER_LOGIC_START_ADDR  ((NORMAL_BOOTLOADER_PHY_START_ADDR*32/34) + LAYOUT_ROM_S_BASE)
#define AES_BOOTLOADER_LOGIC_START_ADDR     ((AES_BOOTLOADER_PHY_START_ADDR*32/34) + LAYOUT_ROM_S_BASE)
#define AES_BOOTLOADER_LOGIC_LENGTH         (AES_BOOTLOADER_PHY_LENGTH*32/34)
#define WRITE_FLASH_DATA_WITH_CPU           (0)

#define RSA_PUBKEY_OFFSET                   (0x11000)
#define AES_ENCRYPTED_OFFSET                (0x22000)
#define AES_ENCRYPTED_PADDING_OFFSET        (0x60)
#define IMAGE_CONTENT_LENGTH                (0x4)

#define RSA_PUBKEY_LENGTH                   (0x400)
#define AES_ENCRYPTED_IMAGE_LENGTH          (0x200)

#define RSA_PUBKEY_HEAD                     "-----BEGIN PUBLIC KEY-----"
#define RSA_PRIVKEY_HEAD                    "-----BEGIN RSA PRIVATE KEY-----"

#define AES_ENCRYPTED_KEY_NUMBER            (6)
#define DEBUG_CODE_SWITCH                   (1)

#define	KEY_BUFFER_LENGTH                   (0x200)
#define AES_KEY_LEGNTH                      (0x20)
#define AES_IV_LENGTH                       (0x10)
#define KEY_LENGTH                          (0x20)

#define AES_KEY_OTP_OFFSET                  (0x60)
#define AES_KEY_OTP_ADDR                    (0x4B100460)
#define MODEL_KEY_OTP_OFFSET                (0x104)
#define MODEL_KEY_OTP_ADDR                  (0x4B100504)
#define BL1_BOOT_PUBLIC_KEY_HASH_OTP_OFFSET (0x128)
#define BL1_BOOT_PUBLIC_KEY_HASH_OTP_ADDR   (0x4B100528)
#define BL2_BOOT_PUBLIC_KEY_HASH_OTP_OFFSET (0x148)
#define BL2_BOOT_PUBLIC_KEY_HASH_OTP_ADDR   (0x4B100548)

#define EK1_OTP_OFFSET                      (0x820)
#define EK1_OTP_ADDR                        (0x4B010820)
#define EK2_OTP_OFFSET                      (0x800)
#define EK2_OTP_ADDR                        (0x4B010800)
#define EK3_OTP_OFFSET                      (0x810)
#define EK3_OTP_ADDR                        (0x4B010810)

#define RANDOM_KEY_OTP_OFFSET               (0x600)
#define RANDOM_KEY_OTP_ADDR                 (0x4B010600)
#define RANDOM_KEY2_OTP_OFFSET              (0x620)
#define RANDOM_KEY2_OTP_ADDR                (0x4B010620)

#define FLASH_AES_OTP_OFFSET                (0xF8)
#define FLASH_AES_OTP2_OFFSET               (0x4)
#define SOC_OTP_AHB_BASEX                   (0x5b010000)

#define BL_DOWNLOAD_WAIT_TIME               (0x500)

#define EFUSE_BYTE0_CONTROL_VALUE           (0x49)
#define EFUSE_BYTE3_CONTROL_VALUE           (0x20)
#define OTP_ACCESS_PERMISSION               (1)
#define SET_EFUSE_CONTROL_FUNCTION          (1)

typedef enum
{
	EFUSE_BYTE0 = 0,
	EFUSE_BYTE1 ,
	EFUSE_BYTE2 ,
	EFUSE_BYTE3 ,
}efuse_id_e;

typedef struct calibration
{
	unsigned char   rsa_enc_aeskey_buf[AES_ENCRYPTED_IMAGE_LENGTH];     // receive rsa_privkey encrypted aeskey from mptool.
	unsigned char   randkey_buf[KEY_BUFFER_LENGTH];                     // receive random key from mptool.
	unsigned char   rsa_privkey_image_buf[RSA_PUBKEY_LENGTH];           // read pubkey(private) image from flash and place it
	unsigned char   rsa_decrypt_buf[KEY_BUFFER_LENGTH];                 // rsa using pubkey decrypt aeskey
	unsigned char   aes_encrypted_image_buf[AES_ENCRYPTED_IMAGE_LENGTH];// read aes encrypted image from flash and place it
	//unsigned char   aes_descrypted_buf[AES_ENCRYPTED_IMAGE_LENGTH];   // using aeskey decrypt the encrypted aeskey and place it 
	size_t          r_aeskey_offset;
	size_t          r_randkey_offset;
	size_t          rsa_pubkey_image_len;
	size_t          aes_encrypted_image_len;
	size_t          inject_key_flag;
}calibration_t;

calibration_t cali_content;
extern struct device *spi_flash_device;

extern int rsa_decrypt_with_public_key(unsigned char *encrypt_text, const char *pubkey_buf,unsigned char *decrypt_text);
extern int rsa_decrypt_with_private_key(unsigned char *encrypt_text, const char *pubkey_buf,unsigned char *decrypt_text);
extern int aes_decrypt_with_aeskey(unsigned char *key_val, unsigned char *iv_val, unsigned char *encrypt_text , unsigned char *plain_text);
extern bk_err_t bk_efuse_read_byte(uint8_t addr, uint8_t *data);
extern bk_err_t bk_efuse_write_byte(uint8_t addr, uint8_t data);

static unsigned int swap_endian(unsigned int num)
{
    return ((num & 0x000000FF) << 24) |
           ((num & 0x0000FF00) << 8) |
           ((num & 0x00FF0000) >> 8) |
           ((num & 0xFF000000) >> 24);
}

static int bl_calibration_init(calibration_t *cali_p)
{
	if(cali_p == NULL)
	{
		PAL_LOG_ERR("bl_calibration_init fail \r\n");
		return BK_FAIL;
	}

	PAL_LOG_DEBUG("aeskey_len :0x%x, randkey_len :0x%x \r\n",sizeof(cali_p->rsa_enc_aeskey_buf),sizeof(cali_p->randkey_buf));

	memset(cali_p->rsa_enc_aeskey_buf,0,sizeof(cali_p->rsa_enc_aeskey_buf));
	memset(cali_p->randkey_buf,0,sizeof(cali_p->randkey_buf));
	memset(cali_p->rsa_privkey_image_buf,0,sizeof(cali_p->rsa_privkey_image_buf));
	memset(cali_p->aes_encrypted_image_buf,0,sizeof(cali_p->aes_encrypted_image_buf));
	cali_p->r_aeskey_offset = 0;
	cali_p->r_randkey_offset = 0;
	cali_p->rsa_pubkey_image_len = 0;
	cali_p->inject_key_flag = 0;

	return BK_OK;
}

// the first four bytes of rsa pubkey image are image lenth.
static int bl_read_rsa_privkey_from_flash(calibration_t *cali_p)
{
	PAL_LOG_DEBUG("%s enter !\n",__FUNCTION__);

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	flash_read(spi_flash_device, RSA_PUBKEY_OFFSET, (uint8_t*)&cali_p->rsa_pubkey_image_len, 4);
	PAL_LOG_DEBUG("rsa_pubkey_len :%d !\n",cali_p->rsa_pubkey_image_len);

	flash_read(spi_flash_device, (RSA_PUBKEY_OFFSET + IMAGE_CONTENT_LENGTH), cali_p->rsa_privkey_image_buf, cali_p->rsa_pubkey_image_len);

	int ret = memcmp(cali_p->rsa_privkey_image_buf, RSA_PRIVKEY_HEAD, strlen(RSA_PRIVKEY_HEAD));
	if(ret == 0)
		PAL_LOG_DEBUG("get rsa public key ok!\n");
	else
		PAL_LOG_DEBUG("get rsa public key fail! \n");

	return ret;
}


// the first four bytes of aes encrypted image are image lenth.
static int bl_read_encrypted_aes_from_flash(calibration_t *cali_p)
{
	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	flash_read(spi_flash_device, AES_ENCRYPTED_OFFSET, (uint8_t*)&cali_p->aes_encrypted_image_len, 4);

	PAL_LOG_DEBUG("%s enter ,aes_encrypted_image_len :%d !\n",__FUNCTION__, cali_p->aes_encrypted_image_len);

	flash_read(spi_flash_device, (AES_ENCRYPTED_OFFSET + IMAGE_CONTENT_LENGTH), cali_p->aes_encrypted_image_buf, cali_p->aes_encrypted_image_len);
#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("aes_encrypted_image_buf");
	for(int i= 0;i<cali_p->aes_encrypted_image_len;i++)
	{
		PAL_LOG_DEBUG("0x%x, ",cali_p->aes_encrypted_image_buf[i]);
		if(i % 16 ==0)
		{
			PAL_LOG_DEBUG("\r\n");
		}
	}
#endif
	return 0;
}

static int bl_operate_rsa_decrypt_with_privkey(calibration_t *cali_p)
{
	int ret_val = BK_FAIL;

	PAL_LOG_DEBUG("%s enter !\n",__FUNCTION__);

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	ret_val = bl_read_rsa_privkey_from_flash(cali_p);

	PAL_LOG_DEBUG("rsa_pubkey_image_buf %s \n",cali_p->rsa_privkey_image_buf);

	if(ret_val == 0)
	{
		rsa_decrypt_with_private_key((unsigned char *)cali_p->rsa_enc_aeskey_buf, (const char *)cali_p->rsa_privkey_image_buf,(unsigned char *)cali_p->rsa_decrypt_buf);
	}
	else
	{
		PAL_LOG_ERR("get privkey fail \r\n");
		return BK_FAIL;
	}

	return BK_OK;
}

static int bl_operate_aes_decrypt_with_aeskey(calibration_t *cali_p)
{
	int ret_val = BK_FAIL;
	unsigned char aes_iv[AES_IV_LENGTH];
	unsigned char aes_key[AES_KEY_LEGNTH];

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	PAL_LOG_DEBUG("%s enter,cali_p :%s \r\n",__FUNCTION__,cali_p->rsa_decrypt_buf);

	memcpy(aes_iv,cali_p->rsa_decrypt_buf,AES_IV_LENGTH);
	memcpy(aes_key,&cali_p->rsa_decrypt_buf[AES_IV_LENGTH],AES_KEY_LEGNTH);

#if DEBUG_CODE_SWITCH
	for(int i =0;i <AES_IV_LENGTH;i++)
	{
		PAL_LOG_DEBUG("aes_iv[%d] =0x%x \r\n",i, aes_iv[i]);
	}

	for(int i =0;i <AES_KEY_LEGNTH;i++)
	{
		PAL_LOG_DEBUG("aes_key[%d] =0x%x \r\n",i,aes_key[i]);
	}
#endif

	ret_val = bl_read_encrypted_aes_from_flash(cali_p);

	if(ret_val == 0)
	{
		ret_val = aes_decrypt_with_aeskey(aes_key,aes_iv,cali_p->aes_encrypted_image_buf,cali_p->aes_encrypted_image_buf);
		PAL_LOG_DEBUG("ret_val :%d \r\n",ret_val);
		if(ret_val != 0)
		{
			return BK_FAIL;
		}
	}

#if DEBUG_CODE_SWITCH
	for(int i = 0;i< 208;i++)
	{
		PAL_LOG_DEBUG("aes_encrypted_image_buf[%d] :0x%x \r\n",i,cali_p->aes_encrypted_image_buf[i]);
	}
#endif
	return BK_OK;
}

static int bl_operate_rsa_and_aes_decrypt(calibration_t *cali_p)
{
	calibration_t *cali_px;
	int ret = BK_FAIL;

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	cali_px = cali_p;
	ret = bl_operate_rsa_decrypt_with_privkey(cali_px);

	if(ret != BK_OK)
	{
		PAL_LOG_ERR("bl_operate_rsa_decrypt_with_pubkey fail \r\n");
		return BK_FAIL;
	}

	PAL_LOG_DEBUG("bl_operate_rsa_decrypt_with_pubkey ok \r\n");

	ret = bl_operate_aes_decrypt_with_aeskey(cali_px);

	if(ret != BK_OK)
	{
		PAL_LOG_ERR("bl_operate_rsa_decrypt_with_pubkey fail \r\n");
		ret = BK_FAIL;
	}

	return ret;
}

static int bl_write_random_key_to_otp2(calibration_t *cali_p)
{
	u32 original_data = 0;
	u32 randkey_buf[KEY_LENGTH]= {0};
	u32 location;
	u32 ret_val = BK_OK;

	PAL_LOG_DEBUG("%s enter \n",__FUNCTION__);

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	location = RANDOM_KEY_OTP_OFFSET/4;
	memcpy(randkey_buf, cali_p->randkey_buf, cali_p->r_randkey_offset);
	bk_otp2_read_otp(location, &original_data);

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("original_data 0x%x, r_randkey_offset :0x%x \n",original_data, cali_p->r_randkey_offset);
#endif

	for(int i = 0; i< (cali_p->r_randkey_offset/4);i++)
	{
		PAL_LOG_DEBUG("key_content: 0x%x \n",(uint32)randkey_buf[i]);

		bk_otp2_write_otp((location +i),(uint32)randkey_buf[i]);

		bk_otp2_read_otp((location +i), &original_data);

		PAL_LOG_DEBUG("randkey_buf: 0x%x ,original_data :0x%x\n",(uint32)randkey_buf[i],original_data);

		ret_val = memcmp(&randkey_buf[i], &original_data, 4);

		if(ret_val != 0)
		{
			PAL_LOG_ERR("write otp fail ,randkey_buf :0x%x, original_data:0x%x \n",(uint32)randkey_buf[i],original_data);
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static int bl_write_flash_aes_key_otp(calibration_t *cali_p, u32 location,u32 len)
{
	u32 original_data = 0;
	u32 buf[KEY_LENGTH]= {0};
	u32 ret_val = BK_OK;

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	u32 op_location = location /4 ;

	memcpy(buf, &cali_p->aes_encrypted_image_buf[cali_p->r_aeskey_offset], len);

	cali_p->r_aeskey_offset += len;

	PAL_LOG_DEBUG("cali_p->r_aeskey_offset: 0x%x \n",cali_p->r_aeskey_offset);

	bk_otp_read_otp(op_location, &original_data);

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("original_data 0x%x \n",original_data);
#endif

	for(int i = ((len/4) -1); i>=0 ;i--)
	{
		PAL_LOG_DEBUG("key_content: 0x%x \n",(uint32)buf[i]);

		u32 little_endian_value = swap_endian((uint32)buf[i]);

		PAL_LOG_DEBUG("little_endian_value: 0x%x \n",little_endian_value);

		bk_otp_write_otp((op_location + (((len/4) -1) -i)),little_endian_value);

		bk_otp_read_otp((op_location + (((len/4) -1) -i)), &original_data);

		ret_val = memcmp(&little_endian_value, &original_data, 4);
		if(ret_val != 0)
		{
			PAL_LOG_DEBUG("write otp fail ,little_endian_value :0x%x, original_data:0x%x \n",little_endian_value,original_data);
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static int bl_write_key_otp(calibration_t *cali_p, u32 location,u32 len)
{
	u32 original_data = 0;
	u32 buf[KEY_LENGTH]= {0};
	u32 ret_val = BK_OK;

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	u32 op_location = location /4 ;

	memcpy(buf, &cali_p->aes_encrypted_image_buf[cali_p->r_aeskey_offset], len);

	cali_p->r_aeskey_offset += len;

	PAL_LOG_DEBUG("cali_p->r_aeskey_offset: 0x%x \n",cali_p->r_aeskey_offset);

	bk_otp_read_otp(op_location, &original_data);

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("original_data 0x%x \n",original_data);
#endif

	for(int i = 0; i< (len/4);i++)
	{
		PAL_LOG_DEBUG("key_content: 0x%x \n",(uint32)buf[i]);

		bk_otp_write_otp((op_location + i),(uint32)buf[i]);
  
		bk_otp_read_otp((op_location + i), &original_data);

		ret_val = memcmp(&buf[i], &original_data, 4);
		if(ret_val != 0)
		{
			PAL_LOG_DEBUG("write otp fail ,buf :0x%x, original_data:0x%x \n",buf[i],original_data);
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static int bl_write_key_otp2(calibration_t *cali_p, u32 location,u32 len)
{
	u32 original_data = 0;
	u32 buf[KEY_LENGTH]= {0};
	u32 ret_val = BK_OK;

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

	u32 op_location = location /4 ;

	memcpy(buf, &cali_p->aes_encrypted_image_buf[cali_p->r_aeskey_offset], len);

	cali_p->r_aeskey_offset += len;

	PAL_LOG_DEBUG("cali_p->r_aeskey_offset: 0x%x \n",cali_p->r_aeskey_offset);

	bk_otp2_read_otp(op_location, &original_data);

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("original_data 0x%x \n",original_data);
#endif

	for(int i = 0; i< (len/4);i++)
	{
		PAL_LOG_DEBUG("key_content: 0x%x \n",(uint32)buf[i]);

		bk_otp2_write_otp((op_location + i),(uint32)buf[i]);
  
		bk_otp2_read_otp((op_location + i), &original_data);

		ret_val = memcmp(&buf[i], &original_data, 4);
		if(ret_val != 0)
		{
			PAL_LOG_DEBUG("write otp fail ,buf :0x%x, original_data:0x%x \n",buf[i],original_data);
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static void bl_close_otp_access_permission(void)
{
	//otp set no access permission.
#if OTP_ACCESS_PERMISSION
	bk_otp_apb_write_permission(OTP_AES_KEY, OTP_NO_ACCESS);
	bk_otp_apb_write_permission(OTP_BL1_BOOT_PUBLIC_KEY_HASH, OTP_NO_ACCESS);
	bk_otp_apb_write_permission(OTP_BL2_BOOT_PUBLIC_KEY_HASH, OTP_NO_ACCESS);
	bk_otp_apb_write_permission(OTP_MODEL_KEY, OTP_NO_ACCESS);
#endif
}

static int bl_write_decrypted_keycontent_to_otp(calibration_t *cali_p)
{
	PAL_LOG_DEBUG("%s enter \n",__FUNCTION__);

	if(cali_p == NULL)
	{
		PAL_LOG_ERR("%s fail \r\n",__FUNCTION__);
	}

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("cali_p->r_aeskey_offset: 0x%x \n",cali_p->r_aeskey_offset);
#endif

	cali_p->r_aeskey_offset = 0;    //reset it 0

	PAL_LOG_DEBUG("write aes key \n");
	bl_write_flash_aes_key_otp(cali_p,AES_KEY_OTP_OFFSET,KEY_LENGTH);

	PAL_LOG_DEBUG("write bl2 public key \n");
	bl_write_key_otp(cali_p,BL1_BOOT_PUBLIC_KEY_HASH_OTP_OFFSET,KEY_LENGTH);

	PAL_LOG_DEBUG("write bl2 public key \n");
	bl_write_key_otp(cali_p,BL2_BOOT_PUBLIC_KEY_HASH_OTP_OFFSET,KEY_LENGTH);

	PAL_LOG_DEBUG("write ek1 \n");
	bl_write_key_otp2(cali_p,EK1_OTP_OFFSET,KEY_LENGTH);

	PAL_LOG_DEBUG("write ek2 \n");
	bl_write_key_otp2(cali_p,EK2_OTP_OFFSET,KEY_LENGTH/2);

	PAL_LOG_DEBUG("write ek3 \n");
	bl_write_key_otp2(cali_p,EK3_OTP_OFFSET,KEY_LENGTH/2);

	PAL_LOG_DEBUG("write model key \n");
	bl_write_key_otp(cali_p,MODEL_KEY_OTP_OFFSET,(KEY_LENGTH/2));

	return BK_OK;
}

static int bl_enable_security_control(void)
{
#if SET_EFUSE_CONTROL_FUNCTION
	u8 efuse_byte0 = 0;
	u8 efuse_byte2 = 0;
	u8 efuse_byte3 = 0;

	bk_efuse_read_byte(EFUSE_BYTE0, &efuse_byte0);
	PAL_LOG_DEBUG("efuse_byte0 :0x%x \n",efuse_byte0);

	bk_efuse_write_byte(EFUSE_BYTE0, (efuse_byte0 | EFUSE_BYTE0_CONTROL_VALUE));

	bk_efuse_read_byte(EFUSE_BYTE2, &efuse_byte2);
	PAL_LOG_DEBUG("efuse_byte2 :0x%x \n",efuse_byte2);

	bk_efuse_read_byte(EFUSE_BYTE3, &efuse_byte3);
	PAL_LOG_DEBUG("efuse_byte3 :0x%x \n",efuse_byte3);

	bk_efuse_write_byte(EFUSE_BYTE3, (efuse_byte3 | EFUSE_BYTE3_CONTROL_VALUE));
#endif
	return BK_OK;
}


int bl_aes_key_handler(void*data, u16 length)
{
	PAL_LOG_DEBUG(" %s enter\r\n",__FUNCTION__);

	memcpy(&cali_content.rsa_enc_aeskey_buf[cali_content.r_aeskey_offset], data, length);
	cali_content.r_aeskey_offset +=length;

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("r_aeskey_offset :0x%x \r\n",cali_content.r_aeskey_offset);
	PAL_LOG_DEBUG("rsa_enc_aeskey_buf\r\n");
	for(int i = 0; i< cali_content.r_aeskey_offset; i++)
	{
		PAL_LOG_DEBUG("0x%x ,",cali_content.rsa_enc_aeskey_buf[i]);
		if(i%16 ==0)
		{
			PAL_LOG_DEBUG("\r\n");
		}
	}

#endif

	return BK_OK;
}

int bl_random_key_handler(void*data, u16 length)
{
	PAL_LOG_DEBUG(" %s enter\r\n",__FUNCTION__);

	memcpy(&cali_content.randkey_buf[cali_content.r_randkey_offset], data, length);
	cali_content.r_randkey_offset +=length;

#if DEBUG_CODE_SWITCH
	PAL_LOG_DEBUG("r_randkey_offset :0x%x \r\n",cali_content.r_randkey_offset);	
	PAL_LOG_DEBUG("cali_content.randkey_buf \r\n");
	for(int i =0; i<length ;i++)
	{
		PAL_LOG_DEBUG("[%d] =0x%x, ",i,cali_content.randkey_buf[i]);
		if(i%16 ==0)
		{
			PAL_LOG_DEBUG("\r\n");
		}
	}
#endif

	return BK_OK;
}

int bl_receive_random_key_over_handler(void*data, u16 length)
{
	PAL_LOG_DEBUG("%s enter \r\n",__FUNCTION__);

	cali_content.inject_key_flag = 1;

	return BK_OK;
}

int bl_secure_boot_enable_handler(void*data, u16 length)
{
	PAL_LOG_DEBUG("%s enter \r\n",__FUNCTION__);

	return BK_OK;
}

__attribute__((section(".iram")))void *mem_cpy(void *dest, const void *src, uint32 count)
{
	char 		*tmp = dest;
	const char 	*s = src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

#if WRITE_FLASH_DATA_WITH_CPU

void move_aes_bootloader_image(void)
{
	uint8_t i = 0;
	unsigned int offset = 0;
	uint8_t input_buf[FLASH_SECTOR_LENGTH] = {0};

	PAL_LOG_DEBUG("do_aes_bootloader_image enter\n");

	flash_erase(spi_flash_device, NORMAL_BOOTLOADER_PHY_START_ADDR, AES_BOOTLOADER_PHY_LENGTH);

	PAL_LOG_DEBUG("do_aes_bootloader_image erase over\n");

	bk_flash_cpu_write_enable();

	for(i = 0; i<(AES_BOOTLOADER_PHY_LENGTH/(sizeof(input_buf))); i++)
	{
		flash_read(spi_flash_device, (AES_BOOTLOADER_PHY_START_ADDR + offset), input_buf, sizeof(input_buf));

		//PAL_LOG_DEBUG("sizeof(buf) :0x%x \r\n",sizeof(input_buf));

		mem_cpy((void *)( NORMAL_BOOTLOADER_LOGIC_START_ADDR + offset),input_buf, sizeof(input_buf));

		offset += sizeof(input_buf);

		//PAL_LOG_DEBUG("offset :0x%x \r\n",offset);
	}

	bk_flash_cpu_write_disable();

	PAL_LOG_DEBUG("move_aes_bootloader_image over\n");

	//reboot

}
#else
void move_aes_bootloader_image(void)
{

	uint8_t i = 0;
	unsigned int offset = 0;
	uint8_t input_buf[FLASH_SECTOR_LENGTH] = {0};

	PAL_LOG_DEBUG("%s enter\n",__FUNCTION__);

	flash_erase(spi_flash_device, NORMAL_BOOTLOADER_PHY_START_ADDR, AES_BOOTLOADER_PHY_LENGTH);

	PAL_LOG_DEBUG("do_aes_bootloader_image erase over\n");

	for(i = 0; i<(AES_BOOTLOADER_PHY_LENGTH/(sizeof(input_buf))); i++)
	{
		flash_read(spi_flash_device, (AES_BOOTLOADER_PHY_START_ADDR + offset), input_buf, sizeof(input_buf));

		//PAL_LOG_DEBUG("sizeof(buf) :0x%x \r\n",sizeof(input_buf));

		flash_write(spi_flash_device,(NORMAL_BOOTLOADER_PHY_START_ADDR + offset),input_buf, sizeof(input_buf));

		offset += sizeof(input_buf);

		//PAL_LOG_DEBUG("offset :0x%x \r\n",offset);
	}

	PAL_LOG_DEBUG("move_aes_bootloader_image over\n");

} 
#endif

void legacy_boot_main_adapt(void)
{
	int restart_t = BL_DOWNLOAD_WAIT_TIME;
	int ret_val = BK_FAIL;

	bl_calibration_init(&cali_content);
	bl_init();
	uart_enable(1,12);
	//spi_initial(SPI_MASTER,SPI_DEFAULT_BAUD);
	PAL_LOG_DEBUG("%s enter \r\n",__FUNCTION__);

	u32 i = DELAY_1MS_CNT * restart_t;//restart_t; // one 2us ;500 1ms
	boot_uart_init(UART_BAUDRATE_DEFAULE,RX_FIFO_THR_COUNT);

	while(i--)//wait 5ms
	{
		boot_rx_frm_handler();
		BK3000_start_wdt(0xA000);
	}

	PAL_LOG_DEBUG("legacy_boot_main_adapt over \r\n");
	PMU_uninit();  //must need this change the way of operating flash.
	hal_wdt_close();

	ret_val = bl_operate_rsa_and_aes_decrypt(&cali_content);

	if((cali_content.inject_key_flag == 1)&&(ret_val == BK_OK))
	{
		ret_val = bl_write_random_key_to_otp2(&cali_content);
		if(ret_val != BK_OK)
		{
			PAL_LOG_ERR("write random_key to otp fail\n");
			wdt_reboot();
		}

		ret_val = bl_write_decrypted_keycontent_to_otp(&cali_content);
		if(ret_val != BK_OK)
		{
			PAL_LOG_ERR("write aes_related_key to otp fail\n");
			wdt_reboot();
		}

		bl_close_otp_access_permission();
		move_aes_bootloader_image();
		bl_enable_security_control();
	}
	PAL_LOG_DEBUG("legacy_boot_main_adapt over \r\n");

	wdt_reboot();

}

/********************************************************************************************************/
/*                                       otp test code                                                  */
/********************************************************************************************************/
#if DEBUG_CODE_WITCH
int bk_otp2_ahb_read(uint32_t location, uint32_t* value)
{
	*value = *((volatile unsigned long *)location);

	return BK_OK;
}

int bk_otp2_ahb_write(uint32_t location, uint32_t value)
{
	 *((volatile unsigned long *)location) = value;

	return BK_OK;
}
#endif


int bl_test_otp_read_write_function(void)
{
	u32 original_data = 0;
	u32 otp_data;
	u32 ahb_original_data = 0;
	u32 ahb_otp_data;

	PAL_LOG_DEBUG("%s enter \n",__FUNCTION__);
	bk_otp_read_otp(FLASH_AES_OTP_OFFSET, &original_data);
	PAL_LOG_DEBUG("original_data 0x%x \n",original_data);

#if DEBUG_CODE_WITCH
	if(!original_data)
	{
		original_data = bk_rand();
		PAL_LOG_DEBUG("data1 0x%x \n",original_data);
		bk_otp_write_otp(FLASH_AES_OTP_OFFSET, original_data);
	}
#endif
	original_data = 0x111;
	bk_otp_overwrite_otp(FLASH_AES_OTP_OFFSET, original_data);
	bk_otp_read_otp(FLASH_AES_OTP_OFFSET, &otp_data);
	PAL_LOG_DEBUG("otp_data 0x%x \n",otp_data);

#if DEBUG_CODE_WITCH
	bk_otp2_ahb_read(SOC_OTP_AHB_BASEX ,&ahb_original_data);
	PAL_LOG_DEBUG("ahb_original_data 0x%x \n",ahb_original_data);

	bk_otp2_ahb_write(SOC_OTP_AHB_BASEX, ahb_original_data);
	bk_otp2_ahb_read(SOC_OTP_AHB_BASEX ,&ahb_otp_data);
	PAL_LOG_DEBUG("ahb_otp_data 0x%x \n",ahb_otp_data);
#endif

	bk_otp2_read_otp(FLASH_AES_OTP2_OFFSET, &ahb_original_data);
	PAL_LOG_DEBUG("ahb_original_data 0x%x \n",ahb_original_data);

	ahb_original_data = 0x3333;
	bk_otp2_overwrite_otp(FLASH_AES_OTP2_OFFSET, ahb_original_data);
	bk_otp2_read_otp(FLASH_AES_OTP2_OFFSET, &ahb_otp_data);
	PAL_LOG_DEBUG("ahb_otp_data 0x%x \n",ahb_otp_data);
	return 0;
}

