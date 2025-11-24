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


/**
 * @brief      Destroy a count util
 *
 * @param[in]      count_util  The count util handle
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t count_util_destroy(count_util_t *count_util);

/**
 * @brief      Create a count util
 *
 * @param[in]      count_util  The count util handle
 * @param[in]      interval    The timer interval(ms)
 * @param[in]      tag         The printf tag
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t count_util_create(count_util_t *count_util, uint32_t interval, char *tag);

/**
 * @brief      Add data size to count util
 *
 * @param[in]      count_util  The count util handle
 * @param[in]      size        The data size(bytes)
 *
 * @return         None
 */
void count_util_add_size(count_util_t *count_util, int32_t size);

#ifdef __cplusplus
}
#endif

