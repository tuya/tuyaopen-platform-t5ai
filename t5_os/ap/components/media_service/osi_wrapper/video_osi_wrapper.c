#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "video_osi_wrapper.h"
#include "avilib_adp.h"

#include <setjmp.h>

#if (CONFIG_FATFS) && (!CONFIG_VFS)
#include "ff.h"
#endif

#if CONFIG_VFS
#include "bk_posix.h"
#endif

extern bk_err_t video_osi_funcs_init(void *config);

static void *malloc_wrapper(size_t size)
{
	return os_malloc(size);
}

static void *zalloc_wrapper(size_t num, size_t size)
{
	return os_zalloc(num * size);
}

static void *realloc_wrapper(void *old_mem, size_t size)
{
	return os_realloc(old_mem, size);
}

static void *psram_malloc_wrapper(size_t size)
{
	return psram_malloc(size);
}

static void *psram_zalloc_wrapper(size_t num, size_t size)
{
	return psram_zalloc(num * size);
}

static void *psram_realloc_wrapper(void *old_mem, size_t size)
{
	return bk_psram_realloc(old_mem, size);
}

static void free_wrapper(void *ptr)
{
	os_free(ptr);
}

static void *memcpy_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy(out, in, n);
}

static void memcpy_word_wrapper(void *out, const void *in, uint32_t n)
{
	return os_memcpy_word(out, in, n);
}

static void assert_wrapper(uint8_t expr, char *expr_s, const char *func)
{
	if (!(expr))
	{
		BK_LOGD(NULL, "(%s) has assert failed at %s.\n", expr_s, func);
		while (1);
	}
}

static uint32_t get_time_wrapper(void)
{
	return rtos_get_time();
}

static int f_open_wrapper(void **fp, const void *path, uint8_t mode)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	*fp = os_malloc(sizeof(FIL));
	return f_open((FIL *)*fp, (char *)path, mode);
#elif (CONFIG_VFS)
	uint32_t flags = 0;

	if (mode == 0x01)
	{
		flags = O_RDONLY;
	}
	else if (mode == 0x02)
	{
		flags = O_WRONLY;
	}
	else if(mode == (0x01 | 0x02))
	{
		flags = O_RDWR;
	} else if (mode == (0x01 | 0x02 |
 0x08))
	{
		flags = O_CREAT | O_RDWR;
	}

	int f = open(path, flags);
	if (f < 0) {
		*fp = NULL;
		return -1;
	} else {
		*fp = (void *)f;
		return 0;
	}
#else
	return -1;
#endif
}

static int f_close_wrapper(void *fp)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	FRESULT ret = FR_OK;
	ret = f_close((FIL *)fp);
	os_free(fp);
	return ret;
#elif (CONFIG_VFS)
	int ret = close((int)fp);
	return ret < 0 ? -1 : 0;
#else
	return -1;
#endif
}

static int f_write_wrapper(void *fp, const void *buff, uint32_t btw, uint32_t *bw)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	return f_write((FIL *)fp, (void *)buff, (UINT)btw, (UINT *)bw);
#elif (CONFIG_VFS)
	*bw = write((int)fp, (void *)buff, btw);
	return *bw < 0 ? -1 : 0;
#else
	return -1;
#endif
}

static int f_read_wrapper(void *fp, const void *buff, uint32_t btr, uint32_t *br)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	return f_read((FIL *)fp, (void *)buff, (UINT)btr, (UINT *)br);
#elif (CONFIG_VFS)
	*br = read((int)fp, (void *)buff, btr);
	return *br < 0 ? -1 : 0;
#else
	return -1;
#endif
}

static int f_lseek_wrapper(void *fp, uint32_t ofs, uint32_t whence)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	if (whence == SEEK_SET) {
		return f_lseek((FIL *)fp, (FSIZE_t)ofs);
	} else if (whence == SEEK_CUR) {
		return f_lseek((FIL *)fp, f_tell((FIL *)fp) + (FSIZE_t)ofs);
	} else if (whence == SEEK_END) {
		return f_lseek((FIL *)fp, f_size((FIL *)fp) + (FSIZE_t)ofs);
	} else {
		return -1;
	}
#elif (CONFIG_VFS)
	return lseek((int)fp, ofs, whence);
#else
	return -1;
#endif
}

static int f_tell_wrapper(void *fp)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	FIL *tmp_fp = (FIL *)fp;
	return f_tell(tmp_fp);
#elif (CONFIG_VFS)
	return (uint32_t)lseek((int)fp, 0, SEEK_CUR);
#else
	return -1;
#endif
}

static int f_size_wrapper(void *fp)
{
#if (CONFIG_FATFS) && (!CONFIG_VFS)
	FIL *tmp_fp = (FIL *)fp;
	return f_size(tmp_fp);
#elif (CONFIG_VFS)
	BK_LOGD(NULL, "Not support yet, please use the stats function!!!\r\n");
	return -1;
#else
	return -1;
#endif
}

static uint32_t get_avi_index_start_addr_wrapper(void)
{
	return AVI_INDEX_START_ADDR;
}

static uint32_t get_avi_index_count_wrapper(void)
{
	return AVI_INDEX_COUNT;
}

static bk_video_osi_funcs_t video_osi_funcs =
{
	.malloc = malloc_wrapper,
	.zalloc = zalloc_wrapper,
	.realloc = realloc_wrapper,
	.psram_malloc = psram_malloc_wrapper,
	.psram_zalloc = psram_zalloc_wrapper,
	.psram_realloc = psram_realloc_wrapper,
	.free = free_wrapper,
	.memcpy = memcpy_wrapper,
	.memcpy_word = memcpy_word_wrapper,

	.log_write = bk_printf_ext,
	.osi_assert = assert_wrapper,
	.get_time = get_time_wrapper,

	.f_open = f_open_wrapper,
	.f_close = f_close_wrapper,
	.f_write = f_write_wrapper,
	.f_read = f_read_wrapper,
	.f_lseek = f_lseek_wrapper,
	.f_tell = f_tell_wrapper,
	.f_size = f_size_wrapper,

	.get_avi_index_start_addr = get_avi_index_start_addr_wrapper,
	.get_avi_index_count = get_avi_index_count_wrapper,
};

bk_err_t bk_video_osi_funcs_init(void)
{
	bk_err_t ret = BK_OK;
	ret = video_osi_funcs_init(&video_osi_funcs);
	return ret;
}

