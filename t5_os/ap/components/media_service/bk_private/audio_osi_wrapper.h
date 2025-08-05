#pragma once


#ifdef  __cplusplus
extern "C" {
#endif

typedef struct
{
	void *(* psram_malloc)(size_t size);
	void *(* malloc)(size_t size);
	void *(* zalloc)(size_t num, size_t size);
	void *(* realloc)(void *old_mem, size_t size);
	void (* free)(void *ptr);
	void *(* memcpy)(void *out, const void *in, uint32_t n);
	void (* memcpy_word)(void *out, const void *in, uint32_t n);
	void *(* memset)(void *b, int c, uint32_t len);
    void *(* memmove)(void *out, const void *in, uint32_t n);
	void (* memset_word)(void *b, int32_t c, uint32_t n);

	void (* log_write)(int level, char *tag, const char *fmt, ...);
	void (* osi_assert)(uint8_t expr, char *expr_s, const char *func);
	uint32_t (* get_time)(void);
} bk_audio_osi_funcs_t;

bk_err_t bk_audio_osi_funcs_init(void);

#ifdef  __cplusplus
}
#endif

