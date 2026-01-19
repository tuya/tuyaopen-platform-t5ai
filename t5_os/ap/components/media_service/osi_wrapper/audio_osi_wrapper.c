#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "audio_osi_wrapper.h"

#include <setjmp.h>

#if CONFIG_FATFS
#include "ff.h"
#endif

extern bk_err_t audio_osi_funcs_init(void *config);

static void *psram_malloc_wrapper(size_t size)
{
#if CONFIG_PSRAM
	return psram_malloc(size);
#else
	return NULL;
#endif
}

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

static void *memset_wrapper(void *b, int c, uint32_t len)
{
	return os_memset(b, c, len);
}

static void *memmove_wrapper(void *out, const void *in, uint32_t n)
{
    return os_memmove(out, in, n);
}

static void memset_word_wrapper(void *b, int32_t c, uint32_t n)
{
	return os_memset_word(b, c, n);
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

static bk_audio_osi_funcs_t audio_osi_funcs =
{
	.psram_malloc = psram_malloc_wrapper,
	.malloc = malloc_wrapper,
	.zalloc = zalloc_wrapper,
	.realloc = realloc_wrapper,
	.free = free_wrapper,
	.memcpy = memcpy_wrapper,
	.memcpy_word = memcpy_word_wrapper,
	.memset = memset_wrapper,
    .memmove = memmove_wrapper,
	.memset_word = memset_word_wrapper,

	.log_write = bk_printf_ext,
	.osi_assert = assert_wrapper,
	.get_time = get_time_wrapper,
};

bk_err_t bk_audio_osi_funcs_init(void)
{
	bk_err_t ret = BK_OK;
	ret = audio_osi_funcs_init(&audio_osi_funcs);
	return ret;
}

