#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vfs_util
{
    char file_name[50];
    int fd;
};

typedef struct vfs_util *vfs_util_handle_t;

/**
 * @brief      Create a vfs util
 *
 * @param[in]      vfs_util  The vfs util handle
 * @param[in]      name      The vfs name, such as "/sd0/mic.pcm"
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t vfs_util_create(vfs_util_handle_t vfs_util, char *name);

/**
 * @brief      Destroy a vfs util
 *
 * @param[in]      vfs_util  The vfs util handle, created by vfs_util_create
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t vfs_util_destroy(vfs_util_handle_t vfs_util);

/**
 * @brief      Transmit data via vfs util
 *
 * @param[in]      vfs_util  The vfs util handle, created by vfs_util_create
 * @param[in]      data_buf   The data buffer
 * @param[in]      len        The data length
 *
 * @return         The result
 *                 - BK_OK: success
 *                 - BK_ERR: failed
 */
bk_err_t vfs_util_tx_data(vfs_util_handle_t vfs_util, void *data_buf, uint32_t len);

#ifdef __cplusplus
}
#endif

