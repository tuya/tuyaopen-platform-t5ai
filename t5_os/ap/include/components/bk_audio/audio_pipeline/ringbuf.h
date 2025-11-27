// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _RINGBUF_H__
#define _RINGBUF_H__

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_OK           (BK_OK)
#define RB_FAIL         (BK_FAIL)
#define RB_DONE         (-2)
#define RB_ABORT        (-3)
#define RB_TIMEOUT      (-4)

typedef struct ringbuf *ringbuf_handle_t;

/**
 * @brief      Create ringbuffer with total size = block_size * n_blocks
 *
 * @param[in]  block_size   Size of each block
 * @param[in]  n_blocks     Number of blocks
 *
 * @return     ringbuf_handle_t
 */
ringbuf_handle_t rb_create(int block_size, int n_blocks);

/**
 * @brief      Cleanup and free all memory created by ringbuf_handle_t
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t rb_destroy(ringbuf_handle_t rb);

/**
 * @brief      Abort waiting until there is space for reading or writing of the ringbuffer
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t rb_abort(ringbuf_handle_t rb);

/**
 * @brief      Reset ringbuffer, clear all values as initial state
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t rb_reset(ringbuf_handle_t rb);

/**
 * @brief      Get total bytes available of Ringbuffer
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     total bytes available
 */
int rb_bytes_available(ringbuf_handle_t rb);

/**
 * @brief      Get the number of bytes that have filled the ringbuffer
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     The number of bytes that have filled the ringbuffer
 */
int rb_bytes_filled(ringbuf_handle_t rb);

/**
 * @brief      Get total size of Ringbuffer (in bytes)
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     total size of Ringbuffer
 */
int rb_get_size(ringbuf_handle_t rb);

/**
 * @brief      Read from Ringbuffer to `buf` with len and wait `tick_to_wait` ticks until enough bytes to read
 *             if the ringbuffer bytes available is less than `len`.
 *             If `buf` argument provided is `NULL`, then ringbuffer do pseudo reads by simply advancing pointers.
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer pointer to read out data
 * @param[in]  len            The length request
 * @param[in]  ticks_to_wait  The ticks to wait
 *
 * @return     Number of bytes read
 */
int rb_read(ringbuf_handle_t rb, char *buf, int len, TickType_t ticks_to_wait);

/**
 * @brief      Write to Ringbuffer from `buf` with `len` and wait `tick_to_wait` ticks until enough space to write
 *             if the ringbuffer space available is less than `len`
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer
 * @param[in]  len            The length
 * @param[in]  ticks_to_wait  The ticks to wait
 *
 * @return     Number of bytes written
 */
int rb_write(ringbuf_handle_t rb, char *buf, int len, TickType_t ticks_to_wait);

/**
 * @brief      Set status of writing to ringbuffer is done
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t rb_done_write(ringbuf_handle_t rb);

/**
 * @brief      Unblock from rb_read
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - BK_OK
 *     - BK_FAIL
 */
bk_err_t rb_unblock_reader(ringbuf_handle_t rb);


#ifdef __cplusplus
}
#endif

#endif
