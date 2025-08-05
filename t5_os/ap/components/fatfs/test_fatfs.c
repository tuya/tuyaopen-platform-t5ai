#include <stdio.h>
#include <string.h>
#include <common/bk_include.h>
#include "diskio.h"
#include "test_fatfs.h"
#include <os/mem.h>
#include "ff.h"

#if (CONFIG_INT_WDT || CONFIG_TASK_WDT)
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif

#define WR_RD_BUF_SIZE          1024
#define TEST_MP3_FILE_NAME      "/Panama_Matteo.mp3"
#define TEST_TXT_FILE_NAME      "/test1.txt"
#define TEST_DUMP_FILE_NAME      "/dump1.txt"

void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);

FATFS *pfs = NULL;

FRESULT scan_files
(
    char *path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT fr;
    DIR dir;
    FILINFO fno;

    fr = f_opendir(&dir, path);                 /* Open the directory */
    if (fr == FR_OK)
    {
        FATFS_LOGD("%s/\r\n", path);
        while (1)
        {
            fr = f_readdir(&dir, &fno);         /* Read a directory item */
            if (fr != FR_OK)
            {
                break;  /* Break on error */
            }
            if (fno.fname[0] == 0)
            {
                break;  /* Break on end of dir */
            }
            if (fno.fattrib & AM_DIR)
            {
                /* It is a directory */
                char *pathTemp = os_malloc(strlen(path)+strlen(fno.fname)+2);
                if(pathTemp == 0)
                {
                    FATFS_LOGE("%s:os_malloc dir fail \r\n", __func__);
                    break;
                }
                sprintf(pathTemp, "%s/%s", path, fno.fname);
                fr = scan_files(pathTemp);      /* Enter the directory */
                if (fr != FR_OK)
                {
                    os_free(pathTemp);
                    pathTemp = 0;
                    break;
		  }
                if(pathTemp)
                {
                    os_free(pathTemp);
                    pathTemp = 0;
	         }
            }
            else
            {
                /* It is a file. */
                FATFS_LOGD("%s/%s\r\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    else
    {
        FATFS_LOGD("f_opendir failed:fr=%d\r\n", fr);
    }

    return fr;
}

void test_mount(DISK_NUMBER number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];

    if (pfs != NULL)
    {
        os_free(pfs);
    }

    pfs = os_malloc(sizeof(FATFS));
	if(NULL == pfs)
	{
		FATFS_LOGD("f_mount malloc failed!\r\n");
		goto failed_mount;
	}

    sprintf(cFileName, "%d:", number);
    fr = f_mount(pfs, cFileName, 1);
    if (fr != FR_OK)
    {
        FATFS_LOGD("f_mount failed:%d\r\n", fr);
    }
    else
    {
        FATFS_LOGD("f_mount OK!\r\n");
    }

failed_mount:
    FATFS_LOGD("----- test_mount %d over  -----\r\n\r\n", number);
}

void test_unmount(DISK_NUMBER number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];
    sprintf(cFileName, "%d:", number);
    fr = f_unmount(number, cFileName, 1);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "f_unmount failed:%d\r\n", fr);
    }
    else
    {
        BK_LOGD(NULL, "f_unmount OK!\r\n");
    }
    BK_LOGD(NULL, "----- test_unmount %d over  -----\r\n\r\n", number);
}
void test_getfree(DISK_NUMBER number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];
    DWORD getnclst;
    sprintf(cFileName, "%d:", number);
    fr = f_getfree(cFileName, &getnclst, &pfs);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "f_getfree failed:%d\r\n", fr);
    }
    else
    {
        DWORD total_MB = (pfs->n_fatent - 2) * (pfs->csize) * (pfs->ssize) / 1024 /1024;  //1024 convert to MB
        DWORD free_MB = getnclst*(pfs->csize)*(pfs->ssize)/1024 /1024;
        os_printf("total size: %lu MB.\r\n", total_MB);
        os_printf("test_getfree getnclst:DEC %d free space: %dMB\r\n", getnclst, free_MB);
        BK_LOGD(NULL, "f_getfree OK!\r\n");
    }
    BK_LOGD(NULL, "----- test_getfree %d over  -----\r\n\r\n", number);
}

void scan_file_system(DISK_NUMBER number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];

    FATFS_LOGD("\r\n----- scan_file_system %d start -----\r\n", number);

    sprintf(cFileName, "%d:", number);
    fr = scan_files(cFileName);
    if (fr != FR_OK)
    {
        FATFS_LOGD("scan_files failed!\r\n");
    }
    else
    {
        FATFS_LOGD("scan_files OK!\r\n");
    }

    FATFS_LOGD("----- scan_file_system %d over  -----\r\n\r\n", number);
}

void test_fatfs(DISK_NUMBER number)
{
    char cFileName[FF_MAX_LFN];
    FIL file;
    FRESULT fr;

    FATFS_LOGD("\r\n----- test_fatfs %d start -----\r\n", number);
    sprintf(cFileName, "%d:%s", number, TEST_MP3_FILE_NAME);
    FATFS_LOGD("f_open \"%s\"\r\n", cFileName);
    fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
    if (fr == FR_OK)
    {
        unsigned int uiTemp = 0;
        unsigned int uiRdTotalLength = 0;
        unsigned char ucRdTemp[WR_RD_BUF_SIZE];

        do
        {
            FATFS_LOGD(".");
            fr = f_read(&file, ucRdTemp, WR_RD_BUF_SIZE, &uiTemp);
            if (fr == FR_OK)
            {
            }
            else
            {
                FATFS_LOGD("f_read failed 1 fr = %d\r\n", fr);
                goto error1;
            }
            uiRdTotalLength += uiTemp;
        }
        while (uiTemp == WR_RD_BUF_SIZE);
        FATFS_LOGD("\r\n");
        FATFS_LOGD("f_read: read total byte = %d\r\n", uiRdTotalLength);

        fr = f_close(&file);
        if (fr != FR_OK)
        {
            FATFS_LOGD("f_close failed 1 fr = %d\r\n", fr);
            goto error1;
        }
        else
        {
            FATFS_LOGD("f_close OK\r\n");
        }
    }
    else
    {
        FATFS_LOGD("f_open failed 1 fr = %d\r\n", fr);
        goto error0;
    }

    FATFS_LOGD("----- test_fatfs %d over  -----\r\n\r\n", number);
    return;

error1:
    f_close(&file);
error0:
    FATFS_LOGD("----- test_fatfs %d failed  -----\r\n\r\n", number);
}


void test_fatfs_read(DISK_NUMBER number, char *filename, uint64_t len)
{
    char cFileName[FF_MAX_LFN];
    FIL file;
    FRESULT fr;
	unsigned char *ucRdTemp = os_malloc(WR_RD_BUF_SIZE);
	uint32_t packet_len = 0;
	uint64_t left_len = 0;

	if(ucRdTemp == 0)
	{
		FATFS_LOGD("%s:os_malloc fail \r\n", __func__);
		goto error1;
	}

    FATFS_LOGD("\r\n----- test_fatfs %d start -----\r\n", number);
	if(filename)
		sprintf(cFileName, "%d:/%s", number, filename);
	else
		sprintf(cFileName, "%d:%s", number, TEST_TXT_FILE_NAME);
    FATFS_LOGD("f_open \"%s\"\r\n", cFileName);
    fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
    if (fr == FR_OK)
    {
        unsigned int uiTemp = 0;
        unsigned int uiRdTotalLength = 0;

		left_len = f_size(&file) < len? f_size(&file) : len;
		FATFS_LOGD("will read left_len = %d \r\n", (uint32_t)left_len);
        do
        {
			packet_len = left_len < WR_RD_BUF_SIZE? left_len : WR_RD_BUF_SIZE;
            FATFS_LOGD("f_read start:%d bytes \r\n", packet_len);
            fr = f_read(&file, ucRdTemp, packet_len, &uiTemp);
            if (fr == FR_OK)
            {
				bk_mem_dump_ex("f_read one cycle", ucRdTemp, packet_len);
				left_len -= uiTemp;
				FATFS_LOGD("f_read one cycle finish:left_len = %d\r\n", (uint32_t)left_len);
            }
            else
            {
                FATFS_LOGD("f_read failed 1 fr = %d\r\n", fr);
                goto error1;
            }
            uiRdTotalLength += uiTemp;

			if(left_len == 0)
				break;
        }
        while (uiTemp == packet_len);

        FATFS_LOGD("f_read: read total byte = %d\r\n", uiRdTotalLength);
        fr = f_close(&file);
        if (fr != FR_OK)
        {
            FATFS_LOGD("f_close failed 1 fr = %d\r\n", fr);
            goto error1;
        }
        else
        {
            FATFS_LOGD("f_close OK\r\n");
        }
    }
    else
    {
        FATFS_LOGD("f_open failed 1 fr = %d\r\n", fr);
    }

    FATFS_LOGD("----- test_fatfs %d over  -----\r\n\r\n", number);
	if(ucRdTemp)
	{
		os_free(ucRdTemp);
		ucRdTemp = 0;
	}

    return;

error1:
    f_close(&file);
	if(ucRdTemp)
	{
		os_free(ucRdTemp);
		ucRdTemp = 0;
	}
}

//File exist:append write contents(<64bytes).
//File not exist:create a file and write contents(<64bytes).
void test_fatfs_append_write(DISK_NUMBER number, char *filename, char *content, uint32_t len)
{
    char cFileName[FF_MAX_LFN];
    FIL file;
    FRESULT fr;

    FATFS_LOGD("\r\n----- test_fatfs %d start -----\r\n", number);
	if(filename)
		sprintf(cFileName, "%d:/%s", number, filename);
	else
		sprintf(cFileName, "%d:/%s", number, TEST_TXT_FILE_NAME);
    FATFS_LOGD("f_open \"%s\"\r\n", cFileName);

    fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
    if (fr == FR_OK)
    {
        unsigned int uiTemp = 0;
        unsigned int uiRdTotalLength = 0;

		len = len > 64? 64:len;
        do
        {
            FATFS_LOGD(".");
            fr = f_write(&file, content, len, &uiTemp);
            if (fr == FR_OK)
            {
            	
            }
            else
            {
                FATFS_LOGD("f_write failed 1 fr = %d\r\n", fr);
                goto error1;
            }
            uiRdTotalLength += uiTemp;
        }
        while (uiRdTotalLength != len);

        fr = f_close(&file);
        if (fr != FR_OK)
        {
            FATFS_LOGD("f_close failed 1 fr = %d\r\n", fr);
            goto error1;
        }
        else
        {
            FATFS_LOGD("f_close OK\r\n");
        }
    }
    else
    {
        FATFS_LOGD("f_open failed 1 fr = %d\r\n", fr);
    }

    FATFS_LOGD("----- test_fatfs %d over  -----\r\n\r\n", number);
    return;

error1:
    f_close(&file);
}

#define TEST_FATFS_PACKET_LEN (1<<10)
void test_fatfs_dump(DISK_NUMBER number, char *filename, uint32_t start_addr, uint32_t dump_len)
{
	FIL file;
	FRESULT fr;
	char cFileName[FF_MAX_LFN];
	unsigned int uiTemp = 0;

	FATFS_LOGD("\r\n----- %s %d start -----\r\n", __func__, number);
	FATFS_LOGD("file_name=%s,start_addr=0x%0x,len=%d \r\n", filename, start_addr, dump_len);

	if(filename)
		sprintf(cFileName, "%d:/%s", number, filename);
	else
		sprintf(cFileName, "%d:%s", number, TEST_DUMP_FILE_NAME);
	FATFS_LOGD("f_open start \"%s\"\r\n", cFileName);

	fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
	if (fr != FR_OK)
	{
		FATFS_LOGD("f_open failed 1 fr = %d\r\n", fr);
		goto exit;
	}

	FATFS_LOGD("f_write start\r\n");
	//write:one time write all contents
	do
	{
		fr = f_write(&file, (uint8_t *)start_addr, dump_len, &uiTemp);
		if (fr != FR_OK)
		{
			FATFS_LOGD("f_write fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		FATFS_LOGD("f_write end len = %d\r\n", uiTemp);
	} while(0);

#if 1
	//set file pointer to head
	FATFS_LOGD("f_lseek start\r\n");
	fr = f_lseek(&file, 0);
	if (fr != FR_OK)
	{
		FATFS_LOGD("f_lseek fail 1 fr = %d\r\n", fr);
		goto exit;
	}
#endif
	FATFS_LOGD("f_close start\r\n");
	//file close
	fr = f_close(&file);
	if (fr != FR_OK)
	{
		FATFS_LOGD("f_close fail 1 fr = %d\r\n", fr);
		goto exit;
	}

exit:
	return;
}

#define TEST_FATFS_MAX_FILE_LEN (1<<20)
#define TEST_FATFS_PACKET_LEN (1<<10)
void test_fatfs_auto_test(DISK_NUMBER number, char *filename, uint32_t len, uint32_t test_count, uint32_t start_addr)
{
	uint32_t i = 0, j = 0, k = 0;
	FIL file;
	FRESULT fr;
	char cFileName[FF_MAX_LFN];
	uint8_t *content_p = (uint8_t *)start_addr;
	unsigned int uiTemp = 0;
	uint8_t *buf_p = 0;
	uint32_t packet_cnt = 0, bytes_cnt = 0;
	FATFS *checkspace_pfs = NULL;
	DWORD freenclst;

	FATFS_LOGV("\r\n----- %s %d start -----\r\n", __func__, number);
	FATFS_LOGV("file_name=%s,len=%d,test_cnt=%d \r\n", filename, len, test_count);

	if(filename)
		sprintf(cFileName, "%d:/%s", number, filename);
	else
		sprintf(cFileName, "%d:%s", number, TEST_TXT_FILE_NAME);
	FATFS_LOGV("f_open \"%s\"\r\n", cFileName);

	len = len < TEST_FATFS_MAX_FILE_LEN? len : TEST_FATFS_MAX_FILE_LEN;
	packet_cnt = len / TEST_FATFS_PACKET_LEN;
	bytes_cnt = len % TEST_FATFS_PACKET_LEN;

	buf_p = os_malloc(TEST_FATFS_PACKET_LEN);
	if (buf_p == NULL)
	{
		FATFS_LOGE("malloc fail \r\n");
		return;
	}

	for(i = 0; i < test_count; i++)
	{
		FATFS_LOGV("test round=%d start \r\n", i);
		fr = f_getfree(cFileName, &freenclst, &checkspace_pfs);
		if(fr != FR_OK)
		{
			FATFS_LOGE("f_getfree failed 1 fr = %d\r\n", fr);
			goto exit;
		} else if(freenclst < SD_MIN_NUMBER_REMAINING_CLUSTERS) {
			//1024 convert KB to MB
			FATFS_LOGE("Insufficient Space! freenclst: %d free_mem:%d MB\r\n",freenclst, freenclst*(checkspace_pfs->csize)*(checkspace_pfs->ssize)/1024/1024);
			goto exit;
		}

		fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_open failed 1 fr = %d\r\n", fr);
			goto exit;
		}

		//set file pointer to head
		fr = f_lseek(&file, 0);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_lseek fail 1 fr = %d\r\n", fr);
			goto exit;
		}
		//write:one time write all contents
		do
		{
			fr = f_write(&file, (uint8_t *)content_p, len, &uiTemp);
			if (fr != FR_OK)
			{
				FATFS_LOGE("f_write fail 1 fr = %d\r\n", fr);
				goto exit;
			}

			FATFS_LOGV("f_write len = %d\r\n", uiTemp);
		}while(0);

		//set file pointer to head
		fr = f_lseek(&file, 0);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_lseek fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		//file close
		fr = f_close(&file);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_close fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		FATFS_LOGV("check round=%d start \r\n", i);
		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_open failed 1 fr = %d\r\n", fr);
			goto exit;
		}

		//set file pointer to head
		fr = f_lseek(&file, 0);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_lseek fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		//read:as the buffer is not enough, so read each time less then TEST_FATFS_PACKET_LEN
		for(j = 0; j < packet_cnt; j++)
		{
			fr = f_read(&file, (uint8_t *)buf_p, TEST_FATFS_PACKET_LEN, &uiTemp);
			if (fr != FR_OK)
			{
				FATFS_LOGE("f_read packet %d fail 1 fr = %d\r\n", j, fr);
				goto exit;
			}
			FATFS_LOGV("f_read len = %d\r\n", uiTemp);

			//compare
			for(k = 0; k < TEST_FATFS_PACKET_LEN; k++)
			{
				if(*(content_p	+ j * TEST_FATFS_PACKET_LEN + k) != (*(buf_p + k)))
				{
					FATFS_LOGE("auto test fail packet[%d] byte[%d] err,source_val=0x%08x,tar_val=0x%08x \r\n", j, k, *(content_p	+ j * TEST_FATFS_PACKET_LEN + k), (*(buf_p + k)));
					goto exit;
				}
			}
		}

		//left contents
		if(bytes_cnt)
		{
			fr = f_read(&file, (uint8_t *)buf_p, bytes_cnt, &uiTemp);
			if (fr != FR_OK)
			{
				FATFS_LOGE("f_read last packet fail 1 fr = %d\r\n", fr);
				goto exit;
			}
			FATFS_LOGV("f_read len = %d\r\n", uiTemp);

			//compare
			for(k = 0; k < bytes_cnt; k++)
			{
				if(*(content_p	+ packet_cnt * TEST_FATFS_PACKET_LEN + k) != (*(buf_p + k)))
				{
					FATFS_LOGE("auto test fail last packet byte[%d] err\r\n", k);
					goto exit;
				}
			}
		}

		//set file pointer to head
		fr = f_lseek(&file, 0);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_lseek fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		//file close
		fr = f_close(&file);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_close fail 1 fr = %d\r\n", fr);
			goto exit;
		}

		FATFS_LOGV("check round=%d end \r\n", i);
	}

	FATFS_LOGD("auto test succ \r\n");

exit:
	os_free(buf_p);
	buf_p = NULL;
}

void test_fatfs_format(DISK_NUMBER number)
{
	FRESULT fr;
	char cFileName[FF_MAX_LFN];
	//unsigned char ucRdTemp[WR_RD_BUF_SIZE];
	unsigned char *ucRdTemp = os_malloc(WR_RD_BUF_SIZE);
	if(ucRdTemp == 0)
	{
		FATFS_LOGE("%s:os_malloc fail \r\n", __func__);
		return;
	}

	FATFS_LOGV("----- test_fatfs_format %d start -----\r\n", number);

	sprintf(cFileName, "%d:", number);
	fr = f_mkfs(cFileName, FM_ANY, 65536, ucRdTemp, WR_RD_BUF_SIZE);
	if (fr != FR_OK)
	{
		os_free(ucRdTemp);
		FATFS_LOGE("f_mkfs fr=%d failed!\r\n", fr);
		return;
	}
	else
	{
		FATFS_LOGD("f_mkfs OK!\r\n");
	}

	FATFS_LOGV("----- test_fatfs_format %d over  -----\r\n", number);

	if(ucRdTemp)
	{
		os_free(ucRdTemp);
		ucRdTemp = 0;
	}
}


#define FATFS_TEST_CONCURRENCY_TASK_MAX_CNT (8)
static beken_thread_t s_fatfs_test_task_handle[FATFS_TEST_CONCURRENCY_TASK_MAX_CNT];

#define FATFS_TEST_CONCURRENCY_MEM_LEN (1024)
#define FATFS_TEST_CONCURRENCY_FILE_LEN (FATFS_TEST_CONCURRENCY_MEM_LEN * 128)	//default:128k bytes
#define FATFS_TEST_CONCURRENCY_FILE_LEN_MAX (FATFS_TEST_CONCURRENCY_MEM_LEN * 1024 * 128)	//MAX:128M bytes

static uint32_t *s_fatfs_test_src_p[FATFS_TEST_CONCURRENCY_TASK_MAX_CNT];
static uint32_t *s_fatfs_test_tar_p[FATFS_TEST_CONCURRENCY_TASK_MAX_CNT];
#define PSRAM_TEST_BYTES_LEN (8<<20)
//arg: ((DISK_NUMBER << 16) | task_id)
static void fatfs_test_concurrency_task(beken_thread_arg_t arg)
{
	uint32_t task_id = ((uint32_t)arg & 0xffff)%FATFS_TEST_CONCURRENCY_TASK_MAX_CNT;
	uint32_t path_id = ((uint32_t)arg >> 16) % DISK_NUMBER_COUNT;
	char cFileName[FF_MAX_LFN];
	FIL file;
	FRESULT fr;
	uint64_t left_len = 0;
	unsigned int uiTemp = 0;
	uint32_t i;

	FATFS_LOGD("path_id=%d,taskid=%d\r\n", path_id, task_id);
	sprintf(cFileName, "%u:/fatfs_test_task_%u.txt", path_id, task_id);
	while (1) {
		//init mem value
		for(i = 0; i < FATFS_TEST_CONCURRENCY_MEM_LEN/4; i++)
			*(s_fatfs_test_src_p[task_id] + i) = (uint32_t)(s_fatfs_test_src_p[task_id] + i);

		//open file
		FATFS_LOGD("f_open \"%s\"\r\n", cFileName);
		fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_open failed fr = %d\r\n", fr);
			continue;
		}

		//write to SDCARD
		left_len = FATFS_TEST_CONCURRENCY_FILE_LEN;
        do
        {
            fr = f_write(&file, s_fatfs_test_src_p[task_id], FATFS_TEST_CONCURRENCY_MEM_LEN, &uiTemp);
            if ((fr == FR_OK) /*&& (uiTemp == FATFS_TEST_CONCURRENCY_MEM_LEN)*/)
            {
                left_len -= FATFS_TEST_CONCURRENCY_MEM_LEN;
            }
            else
            {
                FATFS_LOGE("f_write failed fr = %d,uiTemp=%d,left_len=%d\r\n", fr, uiTemp, left_len);
                continue;
            }
        }
        while (left_len);

        fr = f_close(&file);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_close fail 1 fr = %d\r\n", fr);
			continue;
		}

#if CONFIG_WDT
		bk_wdt_feed();
#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
#endif
#endif

		//read from SDCARD and compare
		FATFS_LOGD("f_open \"%s\"\r\n", cFileName);
		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK)
		{
			FATFS_LOGE("f_open failed fr = %d\r\n", fr);
			continue;
		}

		left_len = FATFS_TEST_CONCURRENCY_FILE_LEN;
		do
		{
			fr = f_read(&file, s_fatfs_test_tar_p[task_id], FATFS_TEST_CONCURRENCY_MEM_LEN, &uiTemp);
			if (fr == FR_OK)
			{
				left_len -= FATFS_TEST_CONCURRENCY_MEM_LEN;
			}
			else
			{
				FATFS_LOGE("f_read failed fr = %d,uiTemp=%d,left_len=%d\r\n", fr, uiTemp, left_len);
				continue;
			}

			//compare
			for(uint32_t i = 0; i < FATFS_TEST_CONCURRENCY_MEM_LEN/4; i++)
			{
				if(*(s_fatfs_test_src_p[task_id] + i) != *(s_fatfs_test_tar_p[task_id] + i))
				{
					FATFS_LOGE("compare failed src = 0x%x,tar=0x%x,i=%d\r\n", *(s_fatfs_test_src_p[task_id] + i), *(s_fatfs_test_tar_p[task_id] + i), i);
				}
			}
		}
		while (left_len);

        fr = f_close(&file);
        if (fr != FR_OK)
        {
            FATFS_LOGD("f_close failed 1 fr = %d\r\n", fr);
            continue;
        }

#if CONFIG_WDT
		bk_wdt_feed();
#if CONFIG_TASK_WDT
		bk_task_wdt_feed();
#endif
#endif

		rtos_delay_milliseconds(20);
	}

	s_fatfs_test_task_handle[task_id] = NULL;
	rtos_delete_thread(NULL);

	if(s_fatfs_test_src_p[task_id])
	{
		os_free(s_fatfs_test_src_p[task_id]);
		s_fatfs_test_src_p[task_id] = NULL;
	}

	if(s_fatfs_test_tar_p[task_id])
	{
		os_free(s_fatfs_test_tar_p[task_id]);
		s_fatfs_test_tar_p[task_id] = NULL;
	}
}

void test_fatfs_concurrency(DISK_NUMBER number, uint32_t task_count, uint32_t file_size)
{
	bk_err_t ret = BK_OK;
	if(task_count > FATFS_TEST_CONCURRENCY_TASK_MAX_CNT)
		task_count = FATFS_TEST_CONCURRENCY_TASK_MAX_CNT;	//max is 8

	for(uint32_t i = 0; i  < task_count; i++)
	{
		if (!s_fatfs_test_task_handle[i])
		{
			ret = rtos_create_thread(&s_fatfs_test_task_handle[i],
									 4,
									 "fatfs_test",
									 (beken_thread_function_t)fatfs_test_concurrency_task,
									 4 * 1024,
									 (beken_thread_arg_t)((number << 16) | i));		//arg: ((DISK_NUMBER << 16) | task_id)
			if (ret != BK_OK)
			{
				s_fatfs_test_task_handle[i] = NULL;
				FATFS_LOGE("Failed to create fatfs test task: %d\r\n", ret);
				return;
			}

			if(s_fatfs_test_src_p[i] == NULL)
				s_fatfs_test_src_p[i] = (uint32_t *)os_malloc(FATFS_TEST_CONCURRENCY_MEM_LEN);
			BK_ASSERT(s_fatfs_test_src_p[i]);

			if(s_fatfs_test_tar_p[i] == NULL)
				s_fatfs_test_tar_p[i] = (uint32_t *)os_malloc(FATFS_TEST_CONCURRENCY_MEM_LEN);
			BK_ASSERT(s_fatfs_test_tar_p[i]);

			FATFS_LOGD("s_fatfs_test_src_p[%d]=0x%x,tar=0x%x\r\n", i, s_fatfs_test_src_p[i], s_fatfs_test_tar_p[i]);
		}
	}
}


// eof

