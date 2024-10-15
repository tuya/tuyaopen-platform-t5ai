/*************************************************************
 * @file        data_move.h
 * @brief       Header file of data_move.c
 * @attention
 */
#ifndef __DATA_MOVE_H__
#define __DATA_MOVE_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RET_DM_SUCCESS           (0)
#define RET_DM_FAILURE           (-1)
#define RET_DM_NO_OTA_DATA       (-2)
#define RET_DM_DATA_ERROR        (-3)

#define DL_VALID_DATA_LEN        (400960)

#define INSTRUCTION_CRC_CHUNK      (34)
#define INS_NO_CRC_CHUNK           (32)
#define CRC_BYTE0_OFFSET           (32)
#define CRC_BYTE1_OFFSET           (33)

#define SM_SECTOR                  (4096)		// 4k
#define DM_CHUNK                   (16384)		// 16k
#define ZLIB_CHUNK                  DM_CHUNK
#define AES_CHUNK                   DM_CHUNK
#define FLASH_WR_CHUNK             (((DM_CHUNK * 34 / 32) + (SM_SECTOR - 1)) / SM_SECTOR * SM_SECTOR)
#define IN_BUF_LEN                 (DM_CHUNK)

#define TINY_AES_IV_LEN                (16)
#define TINY_AES_KEY_LEN               (32)

#define BK_TINY_AES_IV  "0123456789ABCDEF"
#define BK_TINY_AES_KEY "0123456789ABCDEF0123456789ABCDEF"

#define APP_SEC_BASE                   (0x00010000 * 34 / 32) /* 0x11000*/
#define APP_SEC_LEN                    (1083136)
#define DL_SEC_BASE                    (0x00132000)
#define DL_SEC_LEN                     (664 * 1024)

#define SPI_OPREATE_FLASH       0
#define CPU_OPREATE_FLASH      1
/*
 {
	 "part_table": [
		 {
			 "name": "bootloader",
			 "flash_name": "beken_onchip_crc",
			 "offset": "0x00000000",
			 "len": "64K"
		 },
		 {
			 "name": "app",
			 "flash_name": "beken_onchip_crc",
			 "offset": "0x00010000",
			 "len": "1083136"
		 },
		 {
			 "name": "download",
			 "flash_name": "beken_onchip",
			 "offset": "0x00132000",
			 "len": "664K"
		 }
	 ]
 }
 */
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

extern int data_move_handler(void);
uint32_t ota_calc_crc32(uint32_t crc, const void *buf, size_t len);
uint32_t ota_calc_hash(uint32_t hash, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __DATA_MOVE_H__ */
//eof

