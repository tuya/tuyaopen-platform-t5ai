/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#include <common/bk_include.h>
#include <os/mem.h>


#include "ff.h"

#include "sdcard.h"
#include "sdcard_pub.h"

#include "bk_drv_model.h"
#include "bk_sys_ctrl.h"
#include "diskio.h"		/* FatFs lower layer API */

#include "bk_uart.h"
#include "driver_udisk.h"
#include "driver/sd_card.h"

#include <driver/flash.h>
#include <driver/flash_partition.h>

#if (defined CONFIG_FATFS_SPI_0_FLASH || defined CONFIG_FATFS_SPI_1_FLASH)
#include <driver/spi.h>
#include <driver/spi_flash.h>
#endif

#if (defined CONFIG_FATFS_QSPI_0_FLASH || defined CONFIG_FATFS_QSPI_1_FLASH)
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#endif


#if CONFIG_SDCARD_POWER_GPIO_CTRL
#include <driver/gpio.h>
#include "gpio_map.h"
#include "driver/pwr_clk.h"
#define SDCARD_LDO_POWER_CONTROL_WAIT_TIME_MS  10000

#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
static beken_timer_t sdcard_ldo_power_control_timer;
static bool s_disk_io_sdcard_init_flag = 0;
#endif

static bool s_disk_io_sdcard_ldo_power_flag = 0;
#endif

#define SDCARD_READ_FAIL_RETRY_CNT (3)
#define SDCARD_WRITE_FAIL_RETRY_CNT (3)

#if (CONFIG_SDCARD)
static bk_err_t sdcard_ldo_power_enable(uint8_t enable)
{
#if (CONFIG_SDCARD_POWER_GPIO_CTRL)
	if (enable) {
		bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_SDIO, SDCARD_LDO_CTRL_GPIO, GPIO_OUTPUT_STATE_HIGH);
		s_disk_io_sdcard_ldo_power_flag = 1;
	} else {
		bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_SDIO, SDCARD_LDO_CTRL_GPIO, GPIO_OUTPUT_STATE_LOW);
		s_disk_io_sdcard_ldo_power_flag = 0;
	}
#endif

	return BK_OK;
}

static void sdcard_no_operation_timeout_callback(void *param)
{
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
	rtos_stop_timer(&sdcard_ldo_power_control_timer);
	bk_sd_card_deinit();
	sdcard_ldo_power_enable(0);
#endif
}

static void sdcard_operation_timing_initialize_start()
{
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
	if(sdcard_ldo_power_control_timer.handle == NULL)
	{
		rtos_init_timer(&sdcard_ldo_power_control_timer,
						SDCARD_LDO_POWER_CONTROL_WAIT_TIME_MS,
						sdcard_no_operation_timeout_callback,
						(void *)NULL);
	}
	rtos_start_timer(&sdcard_ldo_power_control_timer);
	s_disk_io_sdcard_init_flag = 1;
#endif
}

static void sdcard_operation_timing_uninitialize_stop()
{
#if (CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE)
	rtos_stop_timer(&sdcard_ldo_power_control_timer);
	if(sdcard_ldo_power_control_timer.handle)
	{
		rtos_deinit_timer(&sdcard_ldo_power_control_timer);
		os_memset(&sdcard_ldo_power_control_timer, 0, sizeof(sdcard_ldo_power_control_timer));
	}
	s_disk_io_sdcard_init_flag = 0;
#endif
}

static void sdcard_operation_timing_reload()
{
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
	if(s_disk_io_sdcard_init_flag && !s_disk_io_sdcard_ldo_power_flag) {
		sdcard_ldo_power_enable(1);
		bk_sd_card_init();
		rtos_start_timer(&sdcard_ldo_power_control_timer);
	} else
		rtos_reload_timer(&sdcard_ldo_power_control_timer);
#endif
}

static void sdcard_operation_err_reset()
{
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
	rtos_stop_timer(&sdcard_ldo_power_control_timer);
	rtos_start_timer(&sdcard_ldo_power_control_timer);
	rtos_reload_timer(&sdcard_ldo_power_control_timer);
#endif

	bk_sd_card_deinit();
	bk_sd_card_init();
}
#endif

#define FLASH_SECTOR_SIZE   4096
#define FLASH_SECTOR_COUNT  1024

/* Definitions of physical drive number for each drive */
#define DEV_RAM		DISK_NUMBER_RAM	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_SD		DISK_NUMBER_SDIO_SD	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		DISK_NUMBER_UDISK	/* Example: Map USB MSD to physical drive 2 */
#define DEV_FLASH	DISK_NUMBER_FLASH	/* Example: Map Internal Flash to physical drive 3 */
#define DEV_SPI_0_FLASH	DISK_NUMBER_SPI_0_FLASH	/* Example: Map SPI Flash to physical drive 4 */
#define DEV_SPI_1_FLASH	DISK_NUMBER_SPI_1_FLASH	/* Example: Map SPI Flash to physical drive 5 */
#define DEV_QSPI_0_FLASH	DISK_NUMBER_QSPI_0_FLASH	/* Example: Map SPI Flash to physical drive 4 */
#define DEV_QSPI_1_FLASH	DISK_NUMBER_QSPI_1_FLASH	/* Example: Map SPI Flash to physical drive 5 */

//static DD_HANDLE usb_hdl =DD_HANDLE_UNVALID;
/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;

	switch (pdrv) {
	case DEV_RAM:
		return stat;

	case DEV_SD :
		stat = RES_OK;
		return stat;

	case DEV_USB :
		stat = RES_OK;
		return stat;

	case DEV_FLASH :
		stat = RES_OK;
		return stat;

	default:
		break;
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NOINIT;
	switch (pdrv) {
	case DEV_RAM:
		return stat;

	case DEV_SD :
	{
#if (CONFIG_SDCARD)
		int result = BK_OK;
		sdcard_ldo_power_enable(1);
		result = bk_sd_card_init();
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
		sdcard_operation_timing_initialize_start();
#endif
		if(result != BK_OK) {
			FATFS_LOGI("func %s line %d,  bk_sd_card_init result:%d, do reset\r\n", __func__, __LINE__, result);
			for(uint32_t i = 0; i < SDCARD_READ_FAIL_RETRY_CNT; i++) {
				FATFS_LOGI("%s retry count:%d\r\n", __func__, i);
				bk_sd_card_deinit();
				sdcard_ldo_power_enable(1);
				result = bk_sd_card_init();
				if(result != RES_OK) {
					FATFS_LOGI("%s ERROR result:%d\r\n", __func__, result);
				}
				else
					break;
			}
		}
		if(result == BK_OK)
			stat = 0;
#endif
		return stat;
	}

	case DEV_USB :
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
		if(udisk_init() == USB_RET_OK)
			stat = RES_OK;
#endif
		return stat;

	//Internal flash
	case DEV_FLASH :
		stat = RES_OK;
		return stat;

#if (defined CONFIG_FATFS_SPI_0_FLASH || defined CONFIG_FATFS_SPI_1_FLASH)
#if (defined CONFIG_FATFS_SPI_0_FLASH)
	//external SPI Flash
	case DEV_SPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_SPI_1_FLASH)
	case DEV_SPI_1_FLASH:
#endif
		if(bk_spi_flash_init(SPI_ID_0 + (pdrv-DEV_SPI_0_FLASH)))
			stat = RES_ERROR;	
		else
			stat = RES_OK;
		return stat;
#endif

#if (defined CONFIG_FATFS_QSPI_0_FLASH || defined CONFIG_FATFS_QSPI_1_FLASH)
#if (defined CONFIG_FATFS_QSPI_0_FLASH)
	//external QSPI Flash
	case DEV_QSPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_QSPI_1_FLASH)
	case DEV_QSPI_1_FLASH:
#endif
		if(bk_qspi_flash_init(QSPI_ID_0 + (pdrv-DEV_QSPI_0_FLASH)))
			stat = RES_ERROR;	
		else
			stat = RES_OK;
		return stat;
#endif

	default:
		break;
	}
	return STA_NOINIT;
}

DSTATUS disk_close(void)
{
    FATFS_LOGD("disk_close\r\n");
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_ERROR;
	int result;

	switch (pdrv) {
	case DEV_RAM:
		return res;

	case DEV_SD :
	{
#if (CONFIG_SDCARD)
		sdcard_operation_timing_reload();

		result = bk_sd_card_read_blocks((uint8_t *)buff, sector, count);
		if(result != BK_OK) {
			FATFS_LOGW("func %s line %d,  bk_sd_card_read_blocks result:%d, do reset\r\n", __func__, __LINE__, result);
			for(uint32_t i = 0; i < SDCARD_READ_FAIL_RETRY_CNT; i++) {
				FATFS_LOGW("%s retry count:%d\r\n", __func__, i);
				sdcard_operation_err_reset();
				result = bk_sd_card_read_blocks((uint8_t *)buff, sector, count);
				if(result != RES_OK) {
					FATFS_LOGW("%s ERROR result:%d\r\n", __func__, result);
				}
				else
					break;
			}
		}
		if(result == BK_OK)
			res = RES_OK;
#endif
		return res;
	}

	case DEV_USB :
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
		res = udisk_rd_blk_sync(sector, count, buff);
		if (res != USB_RET_OK) {
			FATFS_LOGD("dev usb disk_read_error res:%d\r\n", res);
			res = RES_ERROR;
		} else
			res = RES_OK;
#endif
		return res;

	case DEV_FLASH :
		result = bk_flash_partition_read(BK_PARTITION_USR_CONFIG, buff,
				sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE * count);
		if(result == BK_OK)
			res = RES_OK;
		return res;

#if (defined CONFIG_FATFS_SPI_0_FLASH || defined CONFIG_FATFS_SPI_1_FLASH)
#if (defined CONFIG_FATFS_SPI_0_FLASH)
	//external SPI Flash
	case DEV_SPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_SPI_1_FLASH)
	case DEV_SPI_1_FLASH:
#endif
		if(bk_spi_flash_read(SPI_ID_0 + (pdrv-DEV_SPI_0_FLASH), sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE))
		{
			FATFS_LOGI("spi_0_flash_read res:%d\r\n", res);
			res = RES_ERROR;
		}
		else
			res = RES_OK;
		return res;
#endif


#if (defined CONFIG_FATFS_QSPI_0_FLASH || defined CONFIG_FATFS_QSPI_1_FLASH)
#if (defined CONFIG_FATFS_QSPI_0_FLASH)
	//external QSPI Flash
	case DEV_QSPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_QSPI_1_FLASH)
	case DEV_QSPI_1_FLASH:
#endif
		if(bk_qspi_flash_read(QSPI_ID_0 + (pdrv-DEV_QSPI_0_FLASH), sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE))
		{
			FATFS_LOGI("qspi_flash_read res:%d\r\n", res);
			res = RES_ERROR;
		}
		else
			res = RES_OK;

		return res;
#endif

	default:
		break;
	}

	return RES_PARERR;
}

static DRESULT sd_disk_check_space_size()
{
	DRESULT res = RES_ERROR;
	FATFS *checkspace_pfs = NULL;
	char cFileName[FF_MAX_LFN];
	DWORD freenclst;
	sprintf(cFileName, "%d:", 1);

	FATFS_LOGV("[+]%s\r\n", __func__);

	res = f_getfree(cFileName, &freenclst, &checkspace_pfs);
	if(res != RES_OK)
		return res;

	if(freenclst < SD_MIN_NUMBER_REMAINING_CLUSTERS) {
		FATFS_LOGD("The space is insufficient\r\n");
		FATFS_LOGD("freenclst: %d free_mem:%d MB\r\n",freenclst, freenclst/SD_CLUSTER_TO_MEM_64KB);
		return RES_NOTRDY;
	}

	FATFS_LOGV("[-]%s\r\n", __func__);

	return res;
}
/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	BYTE *buff,			/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_ERROR;

	switch (pdrv) {
	case DEV_RAM:
		return res;

	case DEV_SD :
	{
#if (CONFIG_SDCARD)
		int result;
		sdcard_operation_timing_reload();

		result = bk_sd_card_write_blocks((uint8_t *)buff, sector, count);
		if(result != BK_OK) {
			for(uint32_t i = 0; i < SDCARD_WRITE_FAIL_RETRY_CNT; i++) {
				sdcard_operation_err_reset();

				result = bk_sd_card_write_blocks((uint8_t *)buff, sector, count);
				if(result != RES_OK) {
					FATFS_LOGD("Check the remaining space!\r\n");
					FATFS_LOGD("Get the value of the remaining space. res: %d\r\n", sd_disk_check_space_size());
				}
				else
					break;
			}
		}
		if(result == BK_OK)
			res = RES_OK;
#endif

		return res;
	}

	case DEV_USB :
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
		// translate the arguments here
		if (udisk_wr_blk_sync(sector, count, buff) !=  USB_RET_OK) {
			FATFS_LOGI("dev usb disk_write_error\r\n");
			res =	RES_ERROR;
		} else
			res = RES_OK;
#endif
		return res;

	case DEV_FLASH :
	{
		bool erase_suc = true;
		// Fatfs flash read only
		for(uint32_t i = 0; i < count; i++)
		{
			if(bk_flash_erase_sector(sector * FLASH_SECTOR_SIZE))
			{
				erase_suc = false;
				break;
			}
		}

		if(erase_suc)
		{
			if(bk_flash_write_bytes(sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE))
				res = RES_ERROR;
			else
				res = RES_OK;
		}
		else
			res = RES_OK;

		return res;
	}

	//external SPI Flash
#if (defined CONFIG_FATFS_SPI_0_FLASH || defined CONFIG_FATFS_SPI_1_FLASH)
#if (defined CONFIG_FATFS_SPI_0_FLASH)
	case DEV_SPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_SPI_1_FLASH)
	case DEV_SPI_1_FLASH:
#endif
		if(bk_spi_flash_erase(SPI_ID_0 + (pdrv-DEV_SPI_0_FLASH), sector * FLASH_SECTOR_SIZE, count * FLASH_SECTOR_SIZE) == BK_OK)
		{
			if(bk_spi_flash_write(SPI_ID_0 + (pdrv-DEV_SPI_0_FLASH), sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE))
				res = RES_ERROR;	
			else
				res = RES_OK;
		}
		else
			res = RES_OK;
		return res;
#endif

#if (defined CONFIG_FATFS_QSPI_0_FLASH || defined CONFIG_FATFS_QSPI_1_FLASH)
#if (defined CONFIG_FATFS_QSPI_0_FLASH)
	//external QSPI Flash
	case DEV_QSPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_QSPI_1_FLASH)
	case DEV_QSPI_1_FLASH:
#endif
		if(bk_qspi_flash_erase(QSPI_ID_0 + (pdrv-DEV_QSPI_0_FLASH), sector * FLASH_SECTOR_SIZE, count * FLASH_SECTOR_SIZE) == BK_OK)
		{
			if(bk_qspi_flash_write(QSPI_ID_0 + (pdrv-DEV_QSPI_0_FLASH), sector * FLASH_SECTOR_SIZE, buff, count * FLASH_SECTOR_SIZE))
				res = RES_ERROR;	
			else
				res = RES_OK;
		}
		else
			res = RES_OK;

		return res;
#endif

	default:
		break;
	}

	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
extern int sdcard_get_size( void );

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	bk_logic_partition_t *partition_info;
	DRESULT res = FR_OK;
	switch (pdrv) {
	case DEV_SD :
#if (CONFIG_SDCARD)
		sdcard_operation_timing_reload();
		// Process of the command for the MMC/SD card
		switch(cmd)
		{
		case CTRL_SYNC:
			{
				res = bk_sd_card_rw_sync();
				if(res != BK_OK)
				{
					FATFS_LOGD("err:sd sync=%d\r\n", res);
				}
			}
			break;
		case GET_SECTOR_SIZE:
			*(WORD *)buff = 512;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(WORD *)buff = 8;
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buff = (uint32_t)bk_sd_card_get_card_size();
			res = RES_OK;
			FATFS_LOGI("sdcard sector cnt=%d\r\n", *(DWORD *)buff);
			break;
		default:
			res = RES_PARERR;
			break;
		}
#endif
		return res;

	case DEV_USB :
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
		// Process of the command the USB drive
        switch(cmd)
        {
        case CTRL_SYNC:
            //    if (SD_WaitReady()==0)res = RES_OK;
            //   else res = RES_ERROR;
            //	Delay(10000);
            //		    	printf("CTRL_SYNC \r\n");
            res = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            res = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            *(WORD *)buff = 8;
            res = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            *(DWORD *)buff = udisk_get_size();
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
            break;
        }
#endif
		return res;

	case DEV_FLASH :
		switch(cmd)
		{
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_SIZE:
			*(WORD *)buff = FLASH_SECTOR_SIZE;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(WORD *)buff = 1;//FLASH_SECTOR_SIZE;
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			partition_info = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
			if (NULL == partition_info) {
				FATFS_LOGI("%s partiion not found.\r\n", __func__);
				break;
			}
			*(DWORD *)buff = partition_info->partition_length / FLASH_SECTOR_SIZE;
			FATFS_LOGI("sector cnt=%d\r\n", *(DWORD *)buff);
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
			break;
		}
		return res;


#if (defined CONFIG_FATFS_SPI_0_FLASH)
	case DEV_SPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_SPI_1_FLASH)
	case DEV_SPI_1_FLASH:
#endif
#if (defined CONFIG_FATFS_QSPI_0_FLASH)
	case DEV_QSPI_0_FLASH :
#endif
#if (defined CONFIG_FATFS_QSPI_1_FLASH)
	case DEV_QSPI_1_FLASH :
#endif
		switch(cmd)
		{
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_SIZE:
			*(WORD *)buff = FLASH_SECTOR_SIZE;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(WORD *)buff = 1;//FLASH_SECTOR_SIZE;
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buff = FLASH_SECTOR_COUNT;    //currently default use 4M size
			FATFS_LOGI("sector cnt=%d\r\n", *(DWORD *)buff);
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
			break;
		}

		return res;

	default:
		break;

	}

	return RES_PARERR;
}

DSTATUS disk_uninitialize ( BYTE pdrv/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = RES_ERROR;
	UINT32 result = RES_ERROR;

	switch (pdrv) {
	case DEV_RAM:
		return stat;

	case DEV_SD :
#if (CONFIG_SDCARD)
	result = bk_sd_card_deinit();
#if CONFIG_SDCARD_POWER_GPIO_CTRL_AUTO_POWERDOWN_WHEN_IDLE
	if(!result)
		sdcard_operation_timing_uninitialize_stop();
#endif
	sdcard_ldo_power_enable(0);
	stat = RES_OK;
#endif
		break;

	case DEV_USB :
		break;

	case DEV_FLASH :
		stat = RES_OK;
		return stat;

#if (defined CONFIG_FATFS_SPI_0_FLASH || defined CONFIG_FATFS_SPI_1_FLASH)
#if (defined CONFIG_FATFS_SPI_0_FLASH)
	//external SPI Flash
	case DEV_SPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_SPI_1_FLASH)
	case DEV_SPI_1_FLASH:
#endif
		if(bk_spi_flash_deinit(SPI_ID_0 + (pdrv-DEV_SPI_0_FLASH)))
			stat = RES_ERROR;	
		else
			stat = RES_OK;
		return stat;
#endif

#if (defined CONFIG_FATFS_QSPI_0_FLASH || defined CONFIG_FATFS_QSPI_1_FLASH)
#if (defined CONFIG_FATFS_QSPI_0_FLASH)
	//external QSPI Flash
	case DEV_QSPI_0_FLASH:
#endif
#if (defined CONFIG_FATFS_QSPI_1_FLASH)
	case DEV_QSPI_1_FLASH:
#endif
		if(bk_qspi_flash_deinit(QSPI_ID_0 + (pdrv-DEV_QSPI_0_FLASH)))
			stat = RES_ERROR;	
		else
			stat = RES_OK;
		return stat;
#endif

	default:
		break;
	}
	if(result != DRV_SUCCESS)
		stat = RES_ERROR;
	else
		stat = RES_OK;

	return stat;
}

uint8 media_is_online(void)
{
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
    return udisk_is_attached();
#endif
	return RES_OK;
}

DRESULT disk_unmount(uint8 pdrv)
{
        //udisk_uninit();
        return RES_OK;
}

// eof

