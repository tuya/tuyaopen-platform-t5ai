#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    beken_timer_t timer;        /**< freertos timer */
    uint32_t timer_interval;    /**< timer interval(ms) */
    uint32_t data_size;         /**< total data size(bytes) */
    char tag[20];               /**< printf tag */
} count_util_t;


bk_err_t count_util_destroy(count_util_t *count_util);
bk_err_t count_util_create(count_util_t *count_util, uint32_t interval, char *tag);
void count_util_add_size(count_util_t *count_util, int32_t size);

#ifdef __cplusplus
}
#endif

