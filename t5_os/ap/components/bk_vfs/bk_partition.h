#ifndef __BK_PARTITION_H_
#define __BK_PARTITION_H_

#include <stdint.h>

#define MOUNT_TARGET_PATH

/*
 *                      **** MOUNT SOURCE path defines ***
 * There are many memories in one project, maybe one memory has many partitions,
 * it should select the source path with these defines.
 * NOTES:Please don't modify these paths which are used by SDK
 *       SDK parses the paths as memory type,and calls init/deinit the selected memory.
 */
#if 1	//Most cases:there is one type memory and one partition in one project, it's for mount source.
//NOTES:Please don't modify the strings, because internal function parses these strings as memory devices type.
#define INTERNAL_FLASH_PATITION_0    "if0"
#define SPI_FLASH_0_PATITION_0       "sf0"
#define SPI_FLASH_1_PATITION_0       "sf1"
#define QSPI_FLASH_0_PATITION_0      "qf0"
#define QSPI_FLASH_1_PATITION_0      "qf1"
//If there are many SDCARDs, BEKEN chips set as SDIO Host.SDCARD or SD-NAND are SD memory type.
#define SD_0_PATITION_0 "sd0"
//If there are many Udisks, BEKEN chips set as USB Host.
#define USB_0_PATITION_0 "ud0"
#else
#define INTERNAL_FLASH_PATITION_0 "if_0_0"
#define INTERNAL_FLASH_PATITION_1 "if_0_1"
#define INTERNAL_FLASH_PATITION_2 "if_0_2"
#define INTERNAL_FLASH_PATITION_3 "if_0_3"

#define SPI_FLASH_0_PATITION_0 "sf_0_0"
#define SPI_FLASH_0_PATITION_1 "sf_0_1"
#define SPI_FLASH_0_PATITION_2 "sf_0_2"
#define SPI_FLASH_0_PATITION_3 "sf_0_3"

#define SPI_FLASH_1_PATITION_0 "sf_1_0"
#define SPI_FLASH_1_PATITION_1 "sf_1_1"
#define SPI_FLASH_1_PATITION_2 "sf_1_2"
#define SPI_FLASH_1_PATITION_3 "sf_1_3"

#define QSPI_FLASH_0_PATITION_0 "qf_0_0"
#define QSPI_FLASH_0_PATITION_1 "qf_0_1"
#define QSPI_FLASH_0_PATITION_2 "qf_0_2"
#define QSPI_FLASH_0_PATITION_3 "qf_0_3"

#define QSPI_FLASH_1_PATITION_0 "qf_1_0"
#define QSPI_FLASH_1_PATITION_1 "qf_1_1"
#define QSPI_FLASH_1_PATITION_2 "qf_1_2"
#define QSPI_FLASH_1_PATITION_3 "qf_1_3"

//If there are many SDCARDs, BEKEN chips set as SDIO Host.SDCARD or SD-NAND are SD memory type.
#define SD_0_PATITION_0 "sd_0_0"
#define SD_0_PATITION_1 "sd_0_1"
#define SD_0_PATITION_2 "sd_0_2"
#define SD_0_PATITION_3 "sd_0_3"

#define SD_1_PATITION_0 "sd_1_0"
#define SD_1_PATITION_1 "sd_1_1"
#define SD_1_PATITION_2 "sd_1_2"
#define SD_1_PATITION_3 "sd_1_3"

//If there are many Udisks, BEKEN chips set as USB Host.
#define USB_0_PATITION_0 "u_0_0"
#define USB_0_PATITION_1 "u_0_1"
#define USB_0_PATITION_2 "u_0_2"
#define USB_0_PATITION_3 "u_0_3"

#define USB_1_PATITION_0 "u_1_0"
#define USB_1_PATITION_1 "u_1_1"
#define USB_1_PATITION_2 "u_1_2"
#define USB_1_PATITION_3 "u_1_3"
#endif

#define FS_TYPE_LFS "lfs"
#define FS_TYPE_FATFS "fatfs"

enum LFS_PARTITION_TYPE {
	LFS_FILE,
	LFS_MEM,
	LFS_FLASH,
	LFS_SPI_FLASH,	//SPI-0 interface
	LFS_SPI_1_FLASH,	//SPI-1 interface

	LFS_QSPI_FLASH,	//QSPI-0 interface
	LFS_QSPI_1_FLASH,	//QSPI-1 interface
};

struct bk_little_fs_partition {
	int part_type;

	const char *mount_path;

	union {
		struct part_file_s {
			const char *file_path;
		} part_file;
		struct part_mem_s {
			uint32_t start_addr;
			uint32_t size;
		} part_mem;
		struct part_flash_s {
			uint32_t start_addr;
			uint32_t size;
            // Modified by TUYA Start
            uint32_t page_size;
            uint32_t block_size;
            // Modified by TUYA End
		} part_flash;
	};
};

int bk_lfs_mount(struct bk_little_fs_partition *parts, int count);

enum FATFS_PARTITION_TYPE {
	FATFS_DEVICE,
	FATFS_RAM,
	FATFS_FILE,
};

#define FATFS_DEV_RAM               "ram"
#define FATFS_DEV_SDCARD            "sdcard"
#define FATFS_DEV_UDISK             "udisk"
#define FATFS_DEV_FLASH             "flash"     //bk internal flash device
#define FATFS_DEV_SPI0_FLASH        "spi0_flash"
#define FATFS_DEV_SPI1_FLASH        "spi1_flash"
#define FATFS_DEV_QSPI0_FLASH       "qspi0_flash"
#define FATFS_DEV_QSPI1_FLASH       "qspi1_flash"


struct bk_fatfs_partition {
	int part_type;

	const char *mount_path;

	union {
		struct {
			const char *device_name;
		} part_dev;
		struct {
			const char *file_path;
		} part_file;
		struct {
			uint32_t start_addr;
			uint32_t size;
		} part_ram;
	};
};

int bk_fatfs_mount(struct bk_fatfs_partition *parts, int count);

#endif

