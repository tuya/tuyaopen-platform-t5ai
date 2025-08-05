#include <stdlib.h>
#include "cli.h"
#include <os/os.h>
#include <components/system.h>
#include "bk_rtos_debug.h"

#if (CONFIG_FATFS)
#include "ff.h"
#include "diskio.h"
#endif

#include <modules/jpeg_decode_sw.h>

#define JPEG_DEC_CNT (sizeof(s_jpegdec_commands) / sizeof(struct cli_command))


#define JPEG_DTCM
#define JPEG_ITCM


#define RANGE_LIMIT(x) (x > 255 ? 255 : (x < 0 ? 0 : x))
#define write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val
#define read_data(addr,val)                  val = *((volatile uint32_t *)(addr))
#define get_addr_data(addr)                  *((volatile uint32_t *)(addr))

extern JPEG_ITCM uint64_t riscv_get_cycle(void);
extern JPEG_ITCM uint64_t riscv_get_mtimer(void);
extern JPEG_ITCM uint64_t riscv_get_instruct_cnt(void);

void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);

static JPEG_DTCM uint64_t volatile	saved_time = 0;
static JPEG_DTCM uint64_t volatile	cur_time = 0;
static JPEG_DTCM uint64_t volatile	saved_sycle_cnt = 0;
static JPEG_DTCM uint64_t volatile	cur_sycle_cnt = 0;
static JPEG_DTCM uint64_t volatile	saved_inst_cnt = 0;
static JPEG_DTCM uint64_t volatile	cur_inst_cnt = 0;

#define jpeg_perfmon(flag) \
{ \
	cur_inst_cnt = riscv_get_instruct_cnt(); \
	cur_sycle_cnt = riscv_get_cycle(); \
	cur_time = riscv_get_mtimer(); \
 \
	saved_time = (cur_time - saved_time) / 26; \
	saved_sycle_cnt = cur_sycle_cnt - saved_sycle_cnt; \
	saved_inst_cnt = cur_inst_cnt - saved_inst_cnt; \
 \
	if (1 == (flag)) { \
		BK_LOGD(NULL,"=====elapse time(%d us).\r\n", (u32)((saved_time) & 0xFFFFFFFF)); \
		BK_LOGD(NULL,"=====diff inst_cnt: %d:%d\r\n", (u32)(saved_inst_cnt >> 32), (u32)(saved_inst_cnt & 0xFFFFFFFF)); \
		BK_LOGD(NULL,"=====diff sycle_cnt: %d:%d\r\n", (u32)(saved_sycle_cnt >> 32), (u32)(saved_sycle_cnt & 0xFFFFFFFF)); \
	} \
 \
	saved_time = riscv_get_mtimer(); \
	saved_sycle_cnt = riscv_get_cycle(); \
	saved_inst_cnt = riscv_get_instruct_cnt(); \
}


//Note the count n is the word count n == sizeof(buf)/sizeof(word)
JPEG_ITCM void memcpy32(uint32_t *out, const uint32_t *in, uint32_t n)
{
	for(int i = 0; i < n; i++)
	{
		write_data((out+i), get_addr_data(in+i));
	}
}


void jpeg_dec_read_sdcard_to_psram(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if (CONFIG_FATFS)
	int ret = 0;
	char *filename;
	uint32_t srcaddr;
	// uint32_t dstaddr;
	uint32_t total_size = 0;
	uint32_t total_word_size = 0;

	filename = argv[1]; //saved file name
	BK_LOGD(NULL,"filename  = %s \r\n", filename);
	srcaddr = os_strtoul(argv[2], NULL, 16) & 0xFFFFFFFF;
	BK_LOGD(NULL,"image p_srcaddr  = %X \r\n", srcaddr);
	// dstaddr = os_strtoul(argv[3], NULL, 16) & 0xFFFFFFFF;
	// BK_LOGD(NULL,"image p_dstaddr  = %X \r\n", dstaddr);
	// uint8_t * p_srcaddr = (uint8_t *)srcaddr;
	// uint8_t * p_dstaddr = (uint8_t *)dstaddr;

	char cFileName[FF_MAX_LFN];
	FIL file;
	FRESULT fr;
	FSIZE_t size_64bit = 0;
	unsigned int uiTemp = 0;

	// step 1: read picture from sd to psram
	sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);
	char *ucRdTemp = NULL; //(char *)p_srcaddr;
	BK_LOGD(NULL,"write to psram addr:  %x \r\n", srcaddr);

	/*open jpeg file*/
	fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK) {
		BK_LOGD(NULL,"open %s fail.\r\n", filename);
		return;
	}

	do {
		size_64bit = f_size(&file);
		total_size = (uint32_t)size_64bit;// total byte
		BK_LOGD(NULL,"read file total_size = %d.\r\n", total_size);

		ucRdTemp = os_malloc(total_size + 4);
		if(NULL == ucRdTemp) {
			BK_LOGD(NULL,"os malloc fail. oom \r\n");
			ret = -1;
			break;
		}
		fr = f_read(&file, ucRdTemp, total_size, &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL,"read file fail.\r\n");
			ret = -2;
			break;
		}

		total_word_size = (total_size >> 2) + (total_size&0x3);

		memcpy32((uint32_t *)srcaddr, (uint32_t *)ucRdTemp, total_word_size);
	} while(0);

	fr = f_close(&file);
	if (fr != FR_OK) {
		BK_LOGD(NULL,"close %s fail!\r\n", filename);
	}

	if(NULL != ucRdTemp) {
		os_free(ucRdTemp);
		ucRdTemp = NULL;
	}

	if(0 == ret) {
		BK_LOGD(NULL,"file read ok\r\n");
	}

#else
	BK_LOGD(NULL,"Not support\r\n");
#endif

}


void jpeg_sw_dec_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_JPEGDEC_SW
	int err = 0;
	uint32_t srcaddr;
	uint32_t dstaddr;
	uint32_t total_size = 0;
	uint8_t  format = 3;

	srcaddr = os_strtoul(argv[1], NULL, 16) & 0xFFFFFFFF;
	BK_LOGD(NULL,"image p_srcaddr  = 0x%X \r\n", srcaddr);
	dstaddr = os_strtoul(argv[2], NULL, 16) & 0xFFFFFFFF;
	BK_LOGD(NULL,"image p_dstaddr  = 0x%X \r\n", dstaddr);
	total_size = os_strtoul(argv[3], NULL, 10) & 0xFFFFFFFF;
	BK_LOGD(NULL,"image total_size  = %d \r\n", total_size);

	if (argc > 4) {
		format = os_strtoul(argv[4], NULL, 10) & 0xFFFFFFFF;
		BK_LOGD(NULL,"image out format  = %d \r\n", format);
	}

	uint8_t * p_srcaddr = (uint8_t *)srcaddr;
	uint8_t * p_dstaddr = (uint8_t *)dstaddr;

	// init jpeg_dec
	err = bk_jpeg_dec_sw_init();
	if (err != kNoErr) {
		BK_LOGD(NULL,"init jpeg_decoder failed\r\n");
		return;
	}

	// set jpeg_dec out format
//	bk_jpeg_dec_sw_set_out_format(format);

	// start jpeg_dec
	BK_LOGD(NULL,"start jpeg_dec.\r\n");
	jpeg_perfmon(0);

	bk_jpeg_dec_sw_register_finish_callback(NULL);
	err = bk_jpeg_dec_sw_start(JPEGDEC_BY_FRAME, p_srcaddr, p_dstaddr, total_size, 0, NULL);
	if (err != kNoErr) {
		BK_LOGD(NULL,"jpeg_decoder failed\r\n");
		return;
	}

	jpeg_perfmon(1);
	BK_LOGD(NULL,"jpeg_dec ok.\r\n");

	bk_jpeg_dec_sw_deinit();
#else
	BK_LOGD(NULL,"Not support\r\n");
#endif


}


void jpeg_dec_write_psram_to_sdcard(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if (CONFIG_FATFS)
	int ret = 0;
	char *filename = NULL;
	char cFileName[FF_MAX_LFN];
	FIL file;
	FRESULT fr;
	unsigned int uiTemp = 0;
	uint32_t total_size = 0;
	// uint32_t total_word_size = 0;
	char *ucRdTemp = NULL;

	filename = argv[1]; //saved file name
	BK_LOGD(NULL,"filename  = %s \r\n", filename);

	total_size = os_strtoul(argv[2], NULL, 10) & 0xFFFFFFFF;
	BK_LOGD(NULL,"image total_size	= %d \r\n", total_size);

	uint32_t paddr = os_strtoul(argv[3], NULL, 16) & 0xFFFFFFFF;
	BK_LOGD(NULL,"read from psram addr = %x \r\n", paddr);

	//	save data to sdcard
	sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

	fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
	if (fr != FR_OK) {
		BK_LOGD(NULL,"open %s fail.\r\n", filename);
		return;
	}

	do {
		ucRdTemp = (char *)paddr;//os_malloc(total_size + 4);
		// if(NULL == ucRdTemp) {
		// 	BK_LOGD(NULL,"os malloc fail. oom \r\n");
		// 	ret = -1;
		// 	break;
		// }
		// total_word_size = (total_size >> 2) + (total_size&0x3);
		// memcpy32((uint32_t *)ucRdTemp, (uint32_t *)paddr, total_word_size);

		fr = f_write(&file, (char *)ucRdTemp, total_size , &uiTemp);
		if (fr != FR_OK) {
			BK_LOGD(NULL,"write %s fail.\r\n", filename);
			ret = -1;
			break;
		}
		BK_LOGD(NULL,"\n");
	}while(0);


	fr = f_close(&file);
	if (fr != FR_OK) {
		BK_LOGD(NULL,"close %s fail!\r\n", filename);
	}

	// if(NULL != ucRdTemp) {
	// 	os_free(ucRdTemp);
	// 	ucRdTemp = NULL;
	// }

	if(0 == ret) {
		BK_LOGD(NULL,"sd card write data to file successful\r\n");
	}

#else
	BK_LOGD(NULL,"Not support\r\n");
#endif
}


static const struct cli_command s_jpegdec_commands[] = {
	{"jpegread", "<filename> <src_addr> Read data from sdcard to psram", jpeg_dec_read_sdcard_to_psram},
	{"jpegwrite", "<filename> <size> <dest_addr> Write data from psram to sdcard", jpeg_dec_write_psram_to_sdcard},
#if CONFIG_JPEGDEC_SW
	{"jpegdec", "<filename> <src_addr> <dest_addr> Test yuv rotate", jpeg_sw_dec_test},
#endif
};

int cli_jpegdec_init(void)
{
	return cli_register_commands(s_jpegdec_commands, JPEG_DEC_CNT);
}

